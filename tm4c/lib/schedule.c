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
    uint32_t next;
    uint32_t intv;
    uint32_t hits;
    uint32_t ticks;
} SchedulerTask;

SchedulerTask taskSlots[SLOT_CNT];

static volatile int cntAlways;
static volatile SchedulerTask * listAlways[SLOT_CNT];

typedef volatile struct QueueNode {
    volatile struct QueueNode *next;
    volatile struct QueueNode *prev;
    SchedulerTask *task;
} QueueNode;

static volatile QueueNode *queueFree;
static volatile QueueNode queuePool[SLOT_CNT];
static volatile QueueNode queueRoot;
#define queueTerminus (&queueRoot)

static void queueInsAfter(QueueNode *pos, SchedulerTask *task);
static void queueInsBefore(QueueNode *pos, SchedulerTask *task);
static void queueRemove(QueueNode *node);

void initScheduler() {
    // initialize queue pointers
    queueRoot.next = queueTerminus;
    queueRoot.prev = queueTerminus;
    queueRoot.task = NULL;
    queueFree = queuePool;
    // initialize queue nodes
    for(int i = 0; i < SLOT_CNT; i++)
        queuePool[i].next = queuePool + i + 1;
    queuePool[SLOT_CNT - 1].next = NULL;
}

static SchedulerTask * allocTask() {
    for(int i = 0; i < SLOT_CNT; i++) {
        if(taskSlots[i].type == TaskDisabled)
            return taskSlots + i;
    }

    // halt and indicate fault if there are no scheduling slots left
    faultBlink(3, 1);
}

static void queueInsAfter(QueueNode *pos, SchedulerTask *task) {
    // get new node
    QueueNode *node = queueFree;
    queueFree = node->next;
    if(node == NULL)
        faultBlink(3, 3);

    // set node task
    node->task = task;
    // link node
    node->prev = pos;
    node->next = pos->next;
    pos->next = node;
    node->next->prev = node;
}

static void queueInsBefore(QueueNode *pos, SchedulerTask *task) {
    // get new node
    QueueNode *node = queueFree;
    queueFree = node->next;
    if(node == NULL)
        faultBlink(3, 3);

    // set node task
    node->task = task;
    // link node
    node->next = pos;
    node->prev = pos->prev;
    pos->prev = node;
    node->prev->next = node;
}

static void queueRemove(QueueNode *node) {
    if(node == queueTerminus)
        faultBlink(3, 2);

    node->prev->next = node->next;
    node->next->prev = node->prev;

    // push onto free stack
    node->task = NULL;
    node->prev = NULL;
    node->next = queueFree;
    queueFree = node;
}

static void scheduleNext(SchedulerTask *task) {
    // advance timestamp
    task->next += task->intv;
    // insert task into schedule queue
    QueueNode *ins = queueRoot.next;
    while(ins->task) {
        if(((int32_t) (task->next - ins->task->next)) < 0) {
            queueInsBefore(ins, task);
            return;
        }
        ins = ins->next;
    }
    queueInsBefore(ins, task);
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
        // execute free-running tasks
        for (int i = 0; i < cntAlways; i++)
            runTask(listAlways[i]);

        // check for schedule triggers
        while(queueRoot.next->task) {
            // check for task triggers
            QueueNode *node = queueRoot.next;
            SchedulerTask * const task = node->task;
            // verify that the task has been triggered
            if (((int32_t) (GPTM0.TAV.raw - task->next)) < 0)
                break;
            // execute the task
            runTask(task);
            // remove from the queue
            if(node->task)
                queueRemove(node);
            // schedule next run
            if (task->type == TaskInterval) {
                // schedule next execution
                scheduleNext(task);
            }
        }
    }
}

void runAlways(SchedulerCallback callback, void *ref) {
    SchedulerTask *task = allocTask();
    task->type = TaskAlways;
    task->callback = callback;
    task->ref = ref;
    task->intv = 0;
    task->next = 0;
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
    scratch.full *= CLK_FREQ;
    task->intv = scratch.ipart;

    // capture current time
    task->next = GPTM0.TAV.raw;
    // schedule execution
    scheduleNext(task);
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

    // compact "always" list
    int j = 0;
    for(int i = 0; i < cntAlways; i++) {
        if(listAlways[i]->type != TaskDisabled)
            listAlways[j++] = listAlways[i];
    }
    cntAlways = j;

    // remove from schedule queue
    QueueNode *next = queueRoot.next;
    while(next->task) {
        QueueNode *node = next;
        next = next->next;

        // remove disabled tasks from the queue
        if(node->task->type == TaskDisabled)
            queueRemove(node);
    }
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

        end += toHex(taskSlots[i].type, 1, '0', end);
        *(end++) = ' ';
        end += toHex((uint32_t) taskSlots[i].callback, 6, '0', end);
        *(end++) = ' ';
        end += toHex((uint32_t) taskSlots[i].ref, 8, '0', end);
        *(end++) = ' ';
        end += fmtFloat(scale * (float) hits, 8, 1, end);
        *(end++) = ' ';
        end += fmtFloat(scale * 0.008f * (float) ticks, 8, 1, end);
        *(end++) = '\n';

        total += ticks;
    }

    end += fmtFloat(scale * 0.008f * (float) total, 35, 1, end);
    *(end++) = '\n';

    return end - buffer;
}
