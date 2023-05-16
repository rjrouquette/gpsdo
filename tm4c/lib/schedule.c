//
// Created by robert on 5/15/23.
//

#include <memory.h>
#include <stdlib.h>
#include "clk/mono.h"
#include "clk/util.h"
#include "format.h"
#include "led.h"
#include "schedule.h"
#include "../hw/interrupts.h"
#include "../hw/timer.h"


struct SchedulerTask;
typedef void (*SchedulerProcessor)(struct SchedulerTask *task);

enum TaskType {
    TaskDisabled,
    TaskAlways,
    TaskInterval,
    TaskOnce
};

#define SLOT_CNT (32)
typedef volatile struct SchedulerTask {
    enum TaskType type;
    SchedulerCallback callback;
    void *ref;
    uint32_t nextIntg, nextFrac;
    uint32_t intvIntg, intvFrac;
    uint32_t hits;
    uint32_t ticks;
} SchedulerTask;

SchedulerTask taskSlots[SLOT_CNT];

static volatile int cntAlways, cntSchedule;
volatile SchedulerTask * listAlways[SLOT_CNT];
volatile SchedulerTask * queueSchedule[SLOT_CNT];


static SchedulerTask * allocTask() {
    for(int i = 0; i < SLOT_CNT; i++) {
        if(taskSlots[i].type == TaskDisabled)
            return taskSlots + i;
    }

    // halt and indicate fault if there are no scheduling slots left
    faultBlink(3, 1);
}

static void scheduleNext(SchedulerTask *task) {
    // advance timestamp
    task->nextIntg += task->intvIntg;
    task->nextFrac += task->intvFrac;
    // correct for fractional overflow
    uint32_t overflow = task->nextFrac;
    overflow -= task->nextIntg * CLK_FREQ;
    while(overflow >= CLK_FREQ) {
        overflow -= CLK_FREQ;
        ++task->nextIntg;
    }
}

/**
 * Run task and update scheduling statistics
 * @param task
 */
static void runTask(SchedulerTask *task) {
    uint32_t start = GPTM0.TAV.raw;
    (*(task->callback))(task->ref);
    task->ticks += GPTM0.TAV.raw - start;
    ++(task->hits);
}

_Noreturn
void runScheduler() {
    // infinite loop
    for (;;) {
        // execute free-running tasks first
        for (int i = 0; i < cntAlways; i++)
            runTask(listAlways[i]);

        // get current time
        __disable_irq();
        const uint32_t nowFrac = GPTM0.TAV.raw;
        const uint32_t nowIntg = clkMonoInt;
        __enable_irq();

        // iterate through task list
        for (int i = 0; i < cntSchedule; i++) {
            SchedulerTask * const task = queueSchedule[i];

            // verify that the task has been triggered
            if ((int32_t) (nowFrac - task->nextFrac) < 0) continue;
            if ((int32_t) (nowIntg - task->nextIntg) < 0) continue;
            // execute the task
            runTask(task);
            // schedule next run
            if (task->type == TaskOnce)
                task->type = TaskDisabled;
            else
                scheduleNext(task);
        }
    }
}

void runAlways(SchedulerCallback callback, void *ref) {
    SchedulerTask *task = allocTask();
    task->type = TaskAlways;
    task->callback = callback;
    task->ref = ref;
    task->intvIntg = 0;
    task->intvFrac = 0;
    task->nextIntg = 0;
    task->nextFrac = 0;
    listAlways[cntAlways++] = task;
}

void runInterval(uint64_t interval, SchedulerCallback callback, void *ref) {
    SchedulerTask *task = allocTask();
    task->type = TaskInterval;
    task->callback = callback;
    task->ref = ref;

    // convert fixed-point interval to raw monotonic domain
    union fixed_32_32 scratch;
    scratch.full = interval;
    task->intvIntg = scratch.ipart;
    scratch.full *= CLK_FREQ;
    task->intvFrac = scratch.ipart;

    // capture current time
    __disable_irq();
    uint32_t snapF = GPTM0.TAV.raw;
    uint32_t snapI = clkMonoInt;
    __enable_irq();
    task->nextFrac = snapF;
    task->nextIntg = snapI;
    // schedule execution
    scheduleNext(task);
    queueSchedule[cntSchedule++] = task;
}

void runOnce(uint64_t when, SchedulerCallback callback, void *ref) {
    SchedulerTask *task = allocTask();
    task->type = TaskOnce;
    task->callback = callback;
    task->ref = ref;
    task->intvIntg = 0;
    task->intvFrac = 0;

    // convert fixed-point to raw monotonic domain
    union fixed_32_32 scratch;
    scratch.full = when;
    task->nextIntg = scratch.ipart;
    scratch.full *= CLK_FREQ;
    task->nextFrac = scratch.ipart;
    queueSchedule[cntSchedule++] = task;
}

void runRemove(SchedulerCallback callback, void *ref) {
    if(ref == NULL) {
        for(int i = 0; i < SLOT_CNT; i++) {
            if(taskSlots[i].callback == callback)
                taskSlots[i].type = TaskDisabled;
        }
    } else {
        for(int i = 0; i < SLOT_CNT; i++) {
            if(taskSlots[i].callback == callback && taskSlots[i].ref == ref)
                taskSlots[i].type = TaskDisabled;
        }
    }

    // compact "always" queue
    int j = 0;
    for(int i = 0; i < cntAlways; i++) {
        if(listAlways[i]->type != TaskDisabled)
            listAlways[j++] = listAlways[i];
    }
    cntAlways = j;

    // compact "schedule" queue
    j = 0;
    for(int i = 0; i < cntSchedule; i++) {
        if(queueSchedule[i]->type != TaskDisabled)
            queueSchedule[j++] = queueSchedule[i];
    }
    cntSchedule = j;
}


static volatile uint32_t prevQuery;
static volatile uint32_t prevHits[SLOT_CNT];
static volatile uint32_t prevTicks[SLOT_CNT];

unsigned runStatus(char *buffer) {
    char *end = buffer;

    uint32_t elapsed = GPTM0.TAV.raw - prevQuery;
    prevQuery += elapsed;
    float scale = 125e6f / (float) elapsed;

    uint32_t total = 0;
    for(int i = 0; i < SLOT_CNT; i++) {
        if(taskSlots[i].type == TaskDisabled)
            continue;

        uint32_t ticks = taskSlots[i].ticks - prevTicks[i];
        prevTicks[i] += ticks;

        uint32_t hits = taskSlots[i].hits - prevHits[i];
        prevHits[i] += hits;

        end += toHex(taskSlots[i].type, 2, '0', end);
        *(end++) = ' ';
        end += toHex((uint32_t) taskSlots[i].callback, 6, '0', end);
        *(end++) = ' ';
        end += toHex((uint32_t) taskSlots[i].ref, 8, '0', end);
        *(end++) = ' ';
        end += fmtFloat(scale * (float) hits, 10, 1, end);
        *(end++) = ' ';
        end += fmtFloat(scale * 0.008f * (float) ticks, 10, 1, end);
        *(end++) = '\n';

        total += ticks;
    }

    end += fmtFloat(scale * 0.008f * (float) total, 10, 1, end);
    *(end++) = '\n';

    return end - buffer;
}
