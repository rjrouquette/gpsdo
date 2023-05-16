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

#define SLOT_CNT (64)
volatile struct SchedulerTask {
    enum TaskType type;
    SchedulerCallback callback;
    void *ref;
    uint32_t nextIntg, nextFrac;
    uint32_t intvIntg, intvFrac;
    uint32_t hits;
    uint32_t time;
} taskSlots[SLOT_CNT];

static volatile int taskCount = 0;


static int allocSlot() {
    int slot = taskCount;
    for(int i = 0; i < taskCount; i++) {
        if(taskSlots[i].type == TaskDisabled) {
            slot = i;
            break;
        }
    }

    // halt and indicate fault if there are no scheduling slots left
    if(slot >= SLOT_CNT)
        faultBlink(3, 1);

    // increase task count if necessary
    if((slot + 1) > taskCount)
        taskCount = slot + 1;
    return slot;
}

static void scheduleNext(volatile struct SchedulerTask *task) {
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
static void runTask(volatile struct SchedulerTask *task) {
    uint32_t start = GPTM0.TAV.raw;
    (*(task->callback))(task->ref);
    task->time += GPTM0.TAV.raw - start;
    ++(task->hits);
}

_Noreturn
void runScheduler() {
    // infinite loop
    for (;;) {
        // get current time
        __disable_irq();
        const uint32_t nowFrac = GPTM0.TAV.raw;
        const uint32_t nowIntg = clkMonoInt;
        __enable_irq();

        // iterate through task list
        for (int i = 0; i < taskCount; i++) {
            // disabled tasks
            if (taskSlots[i].type == TaskDisabled)
                continue;

            // free-running tasks
            if (taskSlots[i].type == TaskAlways) {
                runTask(taskSlots + i);
                continue;
            }

            // verify that the task has been triggered
            if ((int32_t) (nowIntg - taskSlots[i].nextIntg) < 0) continue;
            if ((int32_t) (nowFrac - taskSlots[i].nextFrac) < 0) continue;
            // execute the task
            runTask(taskSlots + i);
            // schedule next run
            if (taskSlots[i].type == TaskOnce)
                taskSlots[i].type = TaskDisabled;
            else
                scheduleNext(taskSlots + i);
        }
    }
}

void runAlways(SchedulerCallback callback, void *ref) {
    int slot = allocSlot();
    taskSlots[slot].type = TaskAlways;
    taskSlots[slot].callback = callback;
    taskSlots[slot].ref = ref;
    taskSlots[slot].intvIntg = 0;
    taskSlots[slot].intvFrac = 0;
    taskSlots[slot].nextIntg = 0;
    taskSlots[slot].nextFrac = 0;
}

void runInterval(uint64_t interval, SchedulerCallback callback, void *ref) {
    int slot = allocSlot();
    taskSlots[slot].type = TaskInterval;
    taskSlots[slot].callback = callback;
    taskSlots[slot].ref = ref;

    // convert fixed-point interval to raw monotonic domain
    union fixed_32_32 scratch;
    scratch.full = interval;
    taskSlots[slot].intvIntg = scratch.ipart;
    scratch.full *= CLK_FREQ;
    taskSlots[slot].intvFrac = scratch.ipart;

    // capture current time
    __disable_irq();
    uint32_t snapF = GPTM0.TAV.raw;
    uint32_t snapI = clkMonoInt;
    __enable_irq();
    taskSlots[slot].nextFrac = snapF;
    taskSlots[slot].nextIntg = snapI;
    // schedule execution
    scheduleNext(taskSlots + slot);
}

void runOnce(uint64_t when, SchedulerCallback callback, void *ref) {
    int slot = allocSlot();
    taskSlots[slot].type = TaskOnce;
    taskSlots[slot].callback = callback;
    taskSlots[slot].ref = ref;
    taskSlots[slot].intvIntg = 0;
    taskSlots[slot].intvFrac = 0;

    // convert fixed-point to raw monotonic domain
    union fixed_32_32 scratch;
    scratch.full = when;
    taskSlots[slot].nextIntg = scratch.ipart;
    scratch.full *= CLK_FREQ;
    taskSlots[slot].nextFrac = scratch.ipart;
}

void runRemove(SchedulerCallback callback, void *ref) {
    if(ref == NULL) {
        for(int i = 0; i < taskCount; i++) {
            if(taskSlots[i].callback == callback)
                taskSlots[i].type = TaskDisabled;
        }
    } else {
        for(int i = 0; i < taskCount; i++) {
            if(taskSlots[i].callback == callback && taskSlots[i].ref == ref)
                taskSlots[i].type = TaskDisabled;
        }
    }

    // compact task list
    while(
            taskCount > 0 &&
            taskSlots[taskCount-1].type == TaskDisabled
    ) --taskCount;
}

unsigned runStatus(char *buffer) {
    char *end = buffer;

    for(int i = 0; i < taskCount; i++) {
        end += toHex(taskSlots[i].type, 2, '0', end);
        *(end++) = ' ';
        end += toHex((uint32_t) taskSlots[i].callback, 6, '0', end);
        *(end++) = ' ';
        end += toHex((uint32_t) taskSlots[i].ref, 8, '0', end);
        *(end++) = ' ';
        end += toDec((uint32_t) taskSlots[i].hits / CLK_MONO_INT(), 10, ' ', end);
        *(end++) = ' ';
        end += toDec((uint32_t) taskSlots[i].time / CLK_MONO_INT(), 10, ' ', end);
        *(end++) = '\n';
    }

    return end - buffer;
}
