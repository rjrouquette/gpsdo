//
// Created by robert on 5/15/23.
//

#include <memory.h>
#include <stddef.h>
#include "../hw/interrupts.h"
#include "clk/mono.h"
#include "format.h"
#include "led.h"
#include "run.h"


#define FAST_FUNC __attribute__((optimize(3)))

typedef enum RunType {
    RunCanceled,
    RunSleep,
    RunPeriodic
} RunType;

typedef struct Task {
    // queue pointers
    struct Task *qNext;
    struct Task *qPrev;

    // task fields
    RunType runType;
    RunCall runCall;
    void *runRef;
    uint32_t runNext;
    uint32_t runIntv;
} Task;

#define SLOT_CNT (32)
static Task *taskFree;
static Task taskPool[SLOT_CNT];
static volatile Task taskQueue;
#define queueHead (taskQueue.qNext)
#define queueRoot ((Task *) &taskQueue)

static Task * allocTask();

FAST_FUNC
static void doNothing(void *ref) { }

__attribute__((always_inline))
static inline uint32_t toMonoRaw(const uint32_t fixed_8_24) {
    uint64_t scratch = fixed_8_24;
    scratch *= CLK_FREQ;
    return scratch >> 24;
}

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
    Task *task = taskFree;
    taskFree = task->qNext;
    __enable_irq();
    memset(task, 0, sizeof(Task));
    return task;
}

FAST_FUNC
static void freeTask(Task *task) {
    // remove from queue
    task->qPrev->qNext = task->qNext;
    task->qNext->qPrev = task->qPrev;
    // push onto free stack
    task->qNext = taskFree;
    taskFree = task;
}

FAST_FUNC
__attribute__((noinline))
static void schedule(Task *task) {
    // locate optimal insertion point
    const uint32_t runNext = task->runNext;
    Task *ins = queueHead;
    while(ins != queueRoot) {
        if(((int32_t) (runNext - ins->runNext)) < 0)
            break;
        ins = ins->qNext;
    }

    // insert task into the scheduling queue
    task->qNext = ins;
    task->qPrev = ins->qPrev;
    ins->qPrev = task;
    task->qPrev->qNext = task;
}

FAST_FUNC
__attribute__((noinline))
static void reschedule(Task *task) {
    if(task->runType == RunCanceled) {
        // delete task it is canceled
        freeTask(task);
        return;
    }

    // remove from queue
    task->qPrev->qNext = task->qNext;
    task->qNext->qPrev = task->qPrev;
    // set next run time
    task->runNext = task->runIntv + (
            (task->runType == RunPeriodic) ? task->runNext : CLK_MONO_RAW
    );
    // schedule next run
    schedule(task);
}

FAST_FUNC
_Noreturn
void runScheduler() {
    // infinite loop
    for (;;) {
        // check for scheduled tasks
        Task *task = queueHead;
        if(((int32_t) (CLK_MONO_RAW - task->runNext)) < 0)
            continue;

        // run the task
        (*(task->runCall))(task->runRef);

        // determine next state
        __disable_irq();
        reschedule(task);
        __enable_irq();
    }
}

void * runSleep(uint32_t delay, RunCall callback, void *ref) {
    Task *task = allocTask();
    task->runType = RunSleep;
    task->runCall = callback;
    task->runRef = ref;

    // convert fixed-point interval to raw monotonic domain
    task->runIntv = toMonoRaw(delay);
    // start after delay
    task->runNext = CLK_MONO_RAW + task->runIntv;
    // add to schedule
    __disable_irq();
    schedule(task);
    __enable_irq();
    return task;
}

void * runPeriodic(uint32_t interval, RunCall callback, void *ref) {
    Task *task = allocTask();
    task->runType = RunPeriodic;
    task->runCall = callback;
    task->runRef = ref;

    // convert fixed-point interval to raw monotonic domain
    task->runIntv = toMonoRaw(interval);
    // start after delay
    task->runNext = CLK_MONO_RAW + task->runIntv;
    // add to schedule
    __disable_irq();
    schedule(task);
    __enable_irq();
    return task;
}

void runAdjust(void *taskHandle, uint32_t newInterval) {
    // set new run interval
    ((Task *) taskHandle)->runIntv = toMonoRaw(newInterval);
}

void runWake(void *taskHandle) {
    __disable_irq();
    Task *task = taskHandle;
    // remove from queue
    task->qPrev->qNext = task->qNext;
    task->qNext->qPrev = task->qPrev;
    // set next run time
    task->runNext = CLK_MONO_RAW;
    // schedule next run
    schedule(task);
    __enable_irq();
}

FAST_FUNC
static void cancelTask(Task *task) {
    // cancel task
    task->runCall = doNothing;
    task->runRef = NULL;
    task->runType = RunCanceled;
    // explicitly delete task if it is not currently running
    if (task != queueHead) {
        freeTask(task);
    }
}

FAST_FUNC
__attribute__((noinline))
static void runCancel_(RunCall callback, void *ref) {
    // match by reference
    if(callback == NULL) {
        Task *iter = queueHead;
        while(iter != queueRoot) {
            Task *task = iter;
            iter = iter->qNext;

            if (task->runRef == ref)
                cancelTask(task);
        }
        return;
    }

    // match by callback
    if(ref == NULL) {
        Task *iter = queueHead;
        while (iter != queueRoot) {
            Task *task = iter;
            iter = iter->qNext;

            if (task->runCall == callback)
                cancelTask(task);
        }
        return;
    }

    // match both callback and reference
    Task *iter = queueHead;
    while (iter != queueRoot) {
        Task *task = iter;
        iter = iter->qNext;

        if ((task->runCall == callback) && (task->runRef == ref))
            cancelTask(task);
    }
}

void runCancel(RunCall callback, void *ref) {
    __disable_irq();
    runCancel_(callback, ref);
    __enable_irq();
}

static const char typeCode[3] = "CSP";

unsigned runStatus(char *buffer) {
    char *end = buffer;

    // header row
    end = append(end, "  Call  Context\n");

    for(int i = 0; i < SLOT_CNT; i++) {
        Task *task = taskPool + i;

        *(end++) = typeCode[task->runType];
        *(end++) = ' ';
        end += toHex((uint32_t) task->runCall, 5, '0', end);
        *(end++) = ' ';
        end += toHex((uint32_t) task->runRef, 8, '0', end);
        *(end++) = '\n';
    }
    return end - buffer;
}
