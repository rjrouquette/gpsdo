//
// Created by robert on 5/15/23.
//

#include <memory.h>
#include <stddef.h>
#include "../hw/interrupts.h"
#include "clk/mono.h"
#include "clk/util.h"
#include "format.h"
#include "led.h"
#include "run.h"


#define FAST_FUNC __attribute__((optimize(3)))

enum RunType {
    RunOnce,
    RunSleep,
    RunPeriodic
};

typedef struct Task {
    // queue pointers
    struct Task *qNext;
    struct Task *qPrev;

    // task fields
    enum RunType runType;
    SchedulerCallback runCall;
    void *runRef;
    uint32_t runNext;
    uint32_t runIntv;
} Task;

#define SLOT_CNT (32)
static Task *taskFree;
static Task taskPool[SLOT_CNT];
static volatile Task taskQueue;
#define queueRoot ((Task *) &taskQueue)

static Task * allocTask();

FAST_FUNC
static void doNothing(void *ref) { }

void initScheduler() {
    // initialize queue nodes
    taskFree = taskPool;
    for(int i = 0; i < SLOT_CNT - 1; i++) {
        taskPool[i].qNext = taskPool + i + 1;
    }

    // initialize queue pointers
    taskQueue.qNext = queueRoot;
    taskQueue.qPrev = queueRoot;
}

static Task * allocTask() {
    __disable_irq();
    if(taskFree == NULL)
        faultBlink(3, 1);
    Task *node = taskFree;
    taskFree = node->qNext;
    __enable_irq();
    memset(node, 0, sizeof(Task));
    return node;
}

FAST_FUNC
static void freeTask(Task *node) {
    // remove from queue
    node->qPrev->qNext = node->qNext;
    node->qNext->qPrev = node->qPrev;
    // push onto free stack
    node->qNext = taskFree;
    taskFree = node;
}

FAST_FUNC
static void reschedule(Task *node) {
    // remove from queue
    node->qPrev->qNext = node->qNext;
    node->qNext->qPrev = node->qPrev;

    // set next run time
    uint32_t nextRun;
    if(node->runType == RunPeriodic)
        nextRun = node->runNext + node->runIntv;
    else if(node->runType == RunSleep)
        nextRun = CLK_MONO_RAW + node->runIntv;
    else
        nextRun = node->runNext;
    node->runNext = nextRun;

    // locate optimal insertion point
    Task *ins = taskQueue.qNext;
    while(ins != queueRoot) {
        if(((int32_t) (nextRun - ins->runNext)) < 0)
            break;
        ins = ins->qNext;
    }

    // insert task into the scheduling queue
    node->qNext = ins;
    node->qPrev = ins->qPrev;
    ins->qPrev = node;
    node->qPrev->qNext = node;
}

FAST_FUNC
_Noreturn
void runScheduler() {
    // infinite loop
    for (;;) {
        // check for scheduled tasks
        Task *task = taskQueue.qNext;
        if(((int32_t) (CLK_MONO_RAW - task->runNext)) < 0)
            continue;

        // run the task
        (*(task->runCall))(task->runRef);

        // determine next state
        __disable_irq();
        if(task->runType == RunOnce) {
            // task is complete
            freeTask(task);
        } else {
            // schedule next run time
            reschedule(task);
        }
        __enable_irq();
    }
}

static void schedule(Task *node) {
    __disable_irq();
    // locate optimal insertion point
    const uint32_t runNext = node->runNext;
    Task *ins = taskQueue.qNext;
    while(ins != queueRoot) {
        if(((int32_t) (runNext - ins->runNext)) < 0)
            break;
        ins = ins->qNext;
    }

    // insert task into the scheduling queue
    node->qNext = ins;
    node->qPrev = ins->qPrev;
    ins->qPrev = node;
    node->qPrev->qNext = node;
    __enable_irq();
}

void * runSleep(uint64_t delay, SchedulerCallback callback, void *ref) {
    Task *node = allocTask();
    node->runType = RunSleep;
    node->runCall = callback;
    node->runRef = ref;

    // convert fixed-point interval to raw monotonic domain
    union fixed_32_32 scratch;
    scratch.full = delay;
    scratch.full *= CLK_FREQ;
    node->runIntv = scratch.ipart;

    // start immediately
    node->runNext = CLK_MONO_RAW;
    // add to schedule
    schedule(node);
    return node;
}

void * runPeriodic(uint64_t interval, SchedulerCallback callback, void *ref) {
    Task *node = allocTask();
    node->runType = RunPeriodic;
    node->runCall = callback;
    node->runRef = ref;

    // convert fixed-point interval to raw monotonic domain
    union fixed_32_32 scratch;
    scratch.full = interval;
    scratch.full *= CLK_FREQ;
    node->runIntv = scratch.ipart;

    // start immediately
    node->runNext = CLK_MONO_RAW;
    // add to schedule
    schedule(node);
    return node;
}

void * runOnce(uint64_t delay, SchedulerCallback callback, void *ref) {
    Task *node = allocTask();
    node->runType = RunOnce;
    node->runCall = callback;
    node->runRef = ref;
    node->runIntv = 0;

    // convert fixed-point interval to raw monotonic domain
    union fixed_32_32 scratch;
    scratch.full = delay;
    scratch.full *= CLK_FREQ;

    // start after delay
    node->runNext = CLK_MONO_RAW + scratch.ipart;
    // add to schedule
    schedule(node);
    return node;
}

void runWake(void *taskHandle) {
    __disable_irq();
    Task *node = taskHandle;
    // remove from queue
    node->qPrev->qNext = node->qNext;
    node->qNext->qPrev = node->qPrev;

    // set next run time
    const uint32_t nextRun = CLK_MONO_RAW;
    node->runNext = nextRun;

    // locate optimal insertion point
    Task *ins = taskQueue.qNext;
    while(ins != queueRoot) {
        if(((int32_t) (nextRun - ins->runNext)) < 0)
            break;
        ins = ins->qNext;
    }

    // insert task into the scheduling queue
    node->qNext = ins;
    node->qPrev = ins->qPrev;
    ins->qPrev = node;
    node->qPrev->qNext = node;
    __enable_irq();
}

void runCancel(SchedulerCallback callback, void *ref) {
    __disable_irq();
    Task *next = taskQueue.qNext;
    while(next != queueRoot) {
        Task *node = next;
        next = next->qNext;

        if(node->runCall == callback) {
            if((ref == NULL) || (node->runRef == ref)) {
                // prevent task from running
                node->runCall = doNothing;
                node->runType = RunOnce;
                // explicitly delete task if it is not currently running
                if(node != taskQueue.qNext) {
                    freeTask(node);
                }
            }
        }
    }
    __enable_irq();
}

static const char typeCode[3] = "OSP";

unsigned runStatus(char *buffer) {
    char *end = buffer;

    // gather tasks
    int taskCount = 0;
    Task *tasks[SLOT_CNT];

    __disable_irq();
    Task *next = taskQueue.qNext;
    while(next != queueRoot) {
        tasks[taskCount++] = next;
        next = next->qNext;
    }
    __enable_irq();

    // header row
    end = append(end, "  Call  Context\n");

    for(int i = 0; i < taskCount; i++) {
        Task *task = tasks[i];

        *(end++) = typeCode[task->runType];
        *(end++) = ' ';
        end += toHex((uint32_t) task->runCall, 5, '0', end);
        *(end++) = ' ';
        end += toHex((uint32_t) task->runRef, 8, '0', end);
        *(end++) = '\n';
    }
    return end - buffer;
}
