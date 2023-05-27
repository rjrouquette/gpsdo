//
// Created by robert on 5/15/23.
//

#include <stddef.h>
#include <memory.h>
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
    uint32_t runHits;
    uint32_t runTicks;
    uint32_t prevHits;
    uint32_t prevTicks;

    // extension fields
    struct Extension {
        SchedulerCallback runCall;
        void *runRef;
        uint32_t countDown;
        uint32_t finalIntv;
    } ext;
} Task;

#define SLOT_CNT (32)
static Task *taskFree;
static Task taskPool[SLOT_CNT];
static volatile Task taskQueue;
#define queueRoot ((Task *) &taskQueue)

static Task * allocTask();

static void doOnceExtended(void *ref);

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
    return node;
}

FAST_FUNC
static void freeTask(Task *node) {
    __disable_irq();
    // remove from queue
    node->qPrev->qNext = node->qNext;
    node->qNext->qPrev = node->qPrev;
    // clear node
    memset((void *) node, 0, sizeof(Task));
    // push onto free stack
    node->qNext = taskFree;
    taskFree = node;
    __enable_irq();
}

FAST_FUNC
static void reschedule(Task *node) {
    __disable_irq();
    // set next run time
    uint32_t nextRun = node->runIntv;
    nextRun += (node->runType == RunPeriodic) ? node->runNext : CLK_MONO_RAW;
    node->runNext = nextRun;

    // locate optimal insertion point
    Task *ins = taskQueue.qNext;
    while(ins != queueRoot) {
        if(((int32_t) (nextRun - ins->runNext)) < 0)
            break;
        ins = ins->qNext;
    }

    // remove from queue
    node->qPrev->qNext = node->qNext;
    node->qNext->qPrev = node->qPrev;
    // insert task into the scheduling queue
    node->qNext = ins;
    node->qPrev = ins->qPrev;
    ins->qPrev = node;
    node->qPrev->qNext = node;
    __enable_irq();
}

FAST_FUNC
_Noreturn
void runScheduler() {
    Task *task = queueRoot;
    uint32_t now = CLK_MONO_RAW;

    // infinite loop
    for (;;) {
        // record time spent on prior task
        const uint32_t prior = now;
        now = CLK_MONO_RAW;
        task->runTicks += now - prior;
        ++(task->runHits);

        // check for scheduled tasks
        task = taskQueue.qNext;
        if(((int32_t) (now - task->runNext)) < 0) {
            // credit idle polling time spent the scheduler
            task = queueRoot;
            continue;
        }

        // run the task
        (*(task->runCall))(task->runRef);

        // determine next state
        if(task->runType == RunOnce) {
            // task is complete
            freeTask(task);
            task = queueRoot;
        } else {
            // schedule next run time
            reschedule(task);
        }
    }
}

static void schedule(Task *node) {
    __disable_irq();
    // locate optimal insertion point
    Task *ins = taskQueue.qNext;
    while(ins != queueRoot) {
        if(((int32_t) (node->runNext - ins->runNext)) < 0)
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

FAST_FUNC
static void doOnceExtended(void *ref) {
    Task *this = (Task *) ref;

    if(this->ext.countDown == 0) {
        // run the task
        (*(this->ext.runCall))(this->ext.runRef);
        // flag for removal from queue
        this->runType = RunOnce;
    }
    else if(--this->ext.countDown == 0) {
        // set final interval once countdown is complete
        this->runIntv = this->ext.finalIntv;
    }
}

static void * runOnceExtended(uint64_t delay, SchedulerCallback callback, void *ref) {
    // allocate queue node
    Task *node = allocTask();
    node->runType = RunPeriodic;
    node->runCall = doOnceExtended;
    node->runRef = node;
    node->runIntv = CLK_FREQ;

    // configure extension
    node->ext.runCall = callback;
    node->ext.runRef = ref;

    // convert fixed-point interval to raw monotonic domain
    union fixed_32_32 scratch;
    scratch.full = delay;
    // set countdown
    node->ext.countDown = scratch.ipart - 1;
    // set final interval
    scratch.ipart = 1;
    scratch.full *= CLK_FREQ;
    node->ext.finalIntv = scratch.ipart;

    // start countdown immediately
    node->runNext = CLK_MONO_RAW + CLK_FREQ;
    // add to schedule
    schedule(node);
    return node;
}

void * runOnce(uint64_t delay, SchedulerCallback callback, void *ref) {
    // check if extended interval support is required
    if((delay >> 32) > 8)
        return runOnceExtended(delay, callback, ref);

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
    // set next run time
    const uint32_t nextRun = CLK_MONO_RAW;
    // schedule task to run immediately
    node->ext.countDown = 0;
    node->runNext = nextRun;

    // locate optimal insertion point
    Task *ins = taskQueue.qNext;
    while(ins != queueRoot) {
        if(((int32_t) (nextRun - ins->runNext)) < 0)
            break;
        ins = ins->qNext;
    }

    // remove from queue
    node->qPrev->qNext = node->qNext;
    node->qNext->qPrev = node->qPrev;
    // insert task into the scheduling queue
    node->qNext = ins;
    node->qPrev = ins->qPrev;
    ins->qPrev = node;
    node->qPrev->qNext = node;
    __enable_irq();
}

void runCancel(SchedulerCallback callback, void *ref) {
    Task *next = taskQueue.qNext;
    while(next != queueRoot) {
        Task *node = next;
        next = next->qNext;

        if(node->runCall == callback) {
            if((ref == NULL) || (node->runRef == ref))
                runRemove(node);
        }
        else if(node->runCall == doOnceExtended) {
            // additional check for extended tasks
            if((node->ext.runCall == callback) && ((ref == NULL) || (node->ext.runRef == ref)))
                runRemove(node);
        }
    }
}

void runRemove(void *taskHandle) {
    // check if the currently running task is being removed
    Task *node = (Task *) taskHandle;
    if(node == taskQueue.qNext) {
        // if so, defer cleanup to main loop
        node->runType = RunOnce;
    } else {
        // if not, explicitly cancel task and free memory
        freeTask(node);
    }
}



static volatile uint32_t prevQuery;
static const char typeCode[3] = "OSP";

unsigned runStatus(char *buffer) {
    char *end = buffer;
    uint32_t elapsed = CLK_MONO_RAW - prevQuery;
    prevQuery += elapsed;
    float scale = 125e6f / (float) elapsed;

    // collect active tasks
    int topList[SLOT_CNT];
    int cnt = 0;
    Task *next = taskQueue.qNext;
    while(next != queueRoot) {
        topList[cnt++] = next - taskPool;
        next = next->qNext;
    }

    for(int i = 0; i < cnt; i++) {
        Task *node = taskPool + topList[i];
        node->prevHits = node->runHits - node->prevHits;
        node->prevTicks = node->runTicks - node->prevTicks;
    }

    // sort active tasks
    for(int i = 0; i < cnt; i++) {
        for(int j = i + 1; j < cnt; j++) {
            int a = topList[i];
            int b = topList[j];
            if(taskPool[a].prevTicks < taskPool[b].prevTicks) {
                topList[i] = b;
                topList[j] = a;
            }
        }
    }

    // header row
    end = append(end, "  Call  Context  Hits     Micros  \n");

    uint32_t totalTicks = 0, totalHits = 0;
    for(int i = 0; i < cnt; i++) {
        Task *node = taskPool + topList[i];

        if(node->runCall == doOnceExtended) {
            *(end++) = 'E';
            *(end++) = ' ';
            end += toHex((uint32_t) node->ext.runCall, 5, '0', end);
            *(end++) = ' ';
            end += toHex((uint32_t) node->ext.runRef, 8, '0', end);
        } else {
            *(end++) = typeCode[node->runType];
            *(end++) = ' ';
            end += toHex((uint32_t) node->runCall, 5, '0', end);
            *(end++) = ' ';
            end += toHex((uint32_t) node->runRef, 8, '0', end);
        }
        *(end++) = ' ';
        end += fmtFloat(scale * (float) node->prevHits, 8, 0, end);
        *(end++) = ' ';
        end += fmtFloat(scale * 0.008f * (float) node->prevTicks, 8, 0, end);
        *(end++) = '\n';

        totalHits += node->prevHits;
        totalTicks += node->prevTicks;
    }

    *(end++) = '\n';
    end += padCopy(17, ' ', end, "Used ", 5);
    end += fmtFloat(scale * (float) totalHits, 8, 0, end);
    *(end++) = ' ';
    end += fmtFloat(scale * 0.008f * (float) totalTicks, 8, 0, end);
    *(end++) = '\n';

    end += padCopy(17, ' ', end, "Idle ", 5);
    end += fmtFloat(scale * (float) (taskQueue.runHits - taskQueue.prevHits), 8, 0, end);
    *(end++) = ' ';
    end += fmtFloat(scale * 0.008f * (float) (taskQueue.runTicks - taskQueue.prevTicks), 8, 0, end);
    *(end++) = '\n';

    // set prior state
    for(int i = 0; i < cnt; i++) {
        Task *node = taskPool + topList[i];
        node->prevHits = node->runHits;
        node->prevTicks = node->runTicks;
    }
    taskQueue.prevHits = taskQueue.runHits;
    taskQueue.prevTicks = taskQueue.runTicks;

    return end - buffer;
}
