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


enum TaskType {
    TaskInvalid,
    TaskSleep,
    TaskPeriodic,
    TaskOnce
};

typedef struct QueueNode {
    // queue pointers
    struct QueueNode *next;
    struct QueueNode *prev;

    // task structure
    struct Task {
        enum TaskType type;
        SchedulerCallback run;
        void *ref;
        uint32_t next;
        uint32_t intv;
        uint32_t hits;
        uint32_t ticks;
        uint32_t prevHits;
        uint32_t prevTicks;
    } task;
} QueueNode;

typedef struct OnceExtended {
    QueueNode *node;
    SchedulerCallback run;
    void *ref;
    uint32_t countDown;
    uint32_t finalIntv;
} OnceExtended;


#define SLOT_CNT (32)
static QueueNode *queueFree;
static QueueNode queuePool[SLOT_CNT];
static volatile QueueNode queueSchedule;

static OnceExtended *extFree;
static OnceExtended extPool[SLOT_CNT];

static QueueNode * allocNode();

static void doOnceExtended(void *ref);

void initScheduler() {
    // initialize queue nodes
    extFree = extPool;
    queueFree = queuePool;
    for(int i = 0; i < SLOT_CNT - 1; i++) {
        extPool[i].ref = extPool + i + 1;
        queuePool[i].next = queuePool + i + 1;
    }

    // initialize queue pointers
    queueSchedule.next = (QueueNode *) &queueSchedule;
    queueSchedule.prev = (QueueNode *) &queueSchedule;
}

static QueueNode * allocNode() {
    if(queueFree == NULL)
        faultBlink(3, 1);
    QueueNode *node = queueFree;
    queueFree = node->next;
    return node;
}

__attribute__((optimize(3)))
static void destroyNode(QueueNode *node) {
    __disable_irq();
    if(node->task.run == doOnceExtended) {
        OnceExtended *ext = (OnceExtended *) node->task.ref;
        ext->ref = extFree;
        extFree = ext;
    }
    // remove from queue
    node->prev->next = node->next;
    node->next->prev = node->prev;
    // clear node
    memset((void *) node, 0, sizeof(QueueNode));
    // push onto free stack
    node->next = queueFree;
    queueFree = node;
    __enable_irq();
}

__attribute__((optimize(3)))
static void reschedule(QueueNode *node) {
    __disable_irq();
    // remove from queue
    node->prev->next = node->next;
    node->next->prev = node->prev;

    // locate optimal insertion point
    QueueNode *ins = queueSchedule.next;
    while(ins->task.type) {
        if(((int32_t) (node->task.next - ins->task.next)) < 0)
            break;
        ins = ins->next;
    }

    // insert task into the scheduling queue
    node->next = ins;
    node->prev = ins->prev;
    ins->prev = node;
    node->prev->next = node;
    __enable_irq();
}

__attribute__((optimize(3)))
_Noreturn
void runScheduler() {
    QueueNode *node = (QueueNode *) &queueSchedule;
    uint32_t now = CLK_MONO_RAW;

    // infinite loop
    for (;;) {
        // record time spent on prior task
        const uint32_t prior = now;
        now = CLK_MONO_RAW;
        node->task.ticks += now - prior;
        ++(node->task.hits);

        // check for scheduled tasks
        node = queueSchedule.next;
        if(((int32_t) (now - node->task.next)) < 0) {
            // credit time spent to the scheduler
            node = (QueueNode *) &queueSchedule;
            continue;
        }

        // run the task
        (*(node->task.run))(node->task.ref);

        // compute next run time
        if(node->task.type == TaskSleep)
            node->task.next = CLK_MONO_RAW + node->task.intv;
        else if(node->task.type == TaskPeriodic)
            node->task.next += node->task.intv;
        else {
            // task is complete
            destroyNode(node);
            // credit time spent to the scheduler
            node = (QueueNode *) &queueSchedule;
            continue;
        }
        reschedule(node);
    }
}

static void schedule(QueueNode *node) {
    __disable_irq();
    // locate optimal insertion point
    QueueNode *ins = queueSchedule.next;
    while(ins->task.type) {
        if(((int32_t) (node->task.next - ins->task.next)) < 0)
            break;
        ins = ins->next;
    }

    // insert task into the scheduling queue
    node->next = ins;
    node->prev = ins->prev;
    ins->prev = node;
    node->prev->next = node;
    __enable_irq();
}

void * runSleep(uint64_t delay, SchedulerCallback callback, void *ref) {
    QueueNode *node = allocNode();
    node->task.type = TaskSleep;
    node->task.run = callback;
    node->task.ref = ref;

    // convert fixed-point interval to raw monotonic domain
    union fixed_32_32 scratch;
    scratch.full = delay;
    scratch.full *= CLK_FREQ;
    node->task.intv = scratch.ipart;

    // start immediately
    node->task.next = CLK_MONO_RAW;
    // add to schedule
    schedule(node);
    return node;
}

void * runPeriodic(uint64_t interval, SchedulerCallback callback, void *ref) {
    QueueNode *node = allocNode();
    node->task.type = TaskPeriodic;
    node->task.run = callback;
    node->task.ref = ref;

    // convert fixed-point interval to raw monotonic domain
    union fixed_32_32 scratch;
    scratch.full = interval;
    scratch.full *= CLK_FREQ;
    node->task.intv = scratch.ipart;

    // start immediately
    node->task.next = CLK_MONO_RAW;
    // add to schedule
    schedule(node);
    return node;
}

__attribute__((optimize(3)))
static void doOnceExtended(void *ref) {
    OnceExtended *this = (OnceExtended *) ref;

    if(this->countDown == 0) {
        // run the task
        (*(this->run))(this->ref);
        // flag for removal from queue
        this->node->task.type = TaskOnce;
    }
    else if(--this->countDown == 0) {
        // set final interval once countdown is complete
        this->node->task.intv = this->finalIntv;
    }
}

static void * runOnceExtended(uint64_t delay, SchedulerCallback callback, void *ref) {
    // allocate extension object
    OnceExtended *extended = extFree;
    extFree = extended->ref;

    // allocate queue node
    QueueNode *node = allocNode();
    node->task.type = TaskPeriodic;
    node->task.run = doOnceExtended;
    node->task.ref = extended;
    node->task.intv = CLK_FREQ;

    // configure extension
    extended->node = node;
    extended->run = callback;
    extended->ref = ref;

    // convert fixed-point interval to raw monotonic domain
    union fixed_32_32 scratch;
    scratch.full = delay;
    // set countdown
    extended->countDown = scratch.ipart - 1;
    // set final interval
    scratch.ipart = 1;
    scratch.full *= CLK_FREQ;
    extended->finalIntv = scratch.ipart;

    // start countdown immediately
    node->task.next = CLK_MONO_RAW + CLK_FREQ;
    // add to schedule
    schedule(node);
    return node;
}

void * runOnce(uint64_t delay, SchedulerCallback callback, void *ref) {
    // check if extended interval support is required
    if((delay >> 32) > 8)
        return runOnceExtended(delay, callback, ref);

    QueueNode *node = allocNode();
    node->task.type = TaskOnce;
    node->task.run = callback;
    node->task.ref = ref;
    node->task.intv = 0;

    // convert fixed-point interval to raw monotonic domain
    union fixed_32_32 scratch;
    scratch.full = delay;
    scratch.full *= CLK_FREQ;

    // start after delay
    node->task.next = CLK_MONO_RAW + scratch.ipart;
    // add to schedule
    schedule(node);
    return node;
}

void runWake(void *taskHandle) {
    QueueNode *node = taskHandle;
    // schedule task to run immediately
    if (node->task.run == doOnceExtended) {
        OnceExtended *ext = (OnceExtended *) node->task.ref;
        ext->countDown = 0;
    }
    node->task.next = CLK_MONO_RAW;
    reschedule(node);
}

void runCancel(SchedulerCallback callback, void *ref) {
    QueueNode *next = queueSchedule.next;
    while(next->task.type) {
        QueueNode *node = next;
        next = next->next;

        if(node->task.run == callback) {
            if((ref == NULL) || (node->task.ref == ref))
                runRemove(node);
        }
        else if(node->task.run == doOnceExtended) {
            // additional check for extended tasks
            OnceExtended *ext = (OnceExtended *) node->task.ref;
            if((ext->run == callback) && ((ref == NULL) || (ext->ref == ref)))
                runRemove(node);
        }
    }
}

void runRemove(void *taskHandle) {
    // check if the currently running task is being removed
    QueueNode *node = (QueueNode *) taskHandle;
    if(node == queueSchedule.next) {
        // if so, defer cleanup to main loop
        node->task.type = TaskOnce;
    } else {
        // if not, explicitly cancel task and free memory
        destroyNode(node);
    }
}



static volatile uint32_t prevQuery, idleHits, idleTicks;
static volatile int topList[SLOT_CNT];

static const char typeCode[4] = {
        '-', 'S', 'P', 'O'
};

unsigned runStatus(char *buffer) {
    char *end = buffer;
    uint32_t elapsed = CLK_MONO_RAW - prevQuery;
    prevQuery += elapsed;
    float scale = 125e6f / (float) elapsed;

    // collect active tasks
    int cnt = 0;
    QueueNode *next = queueSchedule.next;
    while(next->task.type) {
        topList[cnt++] = next - queuePool;
        next = next->next;
    }

    for(int i = 0; i < cnt; i++) {
        QueueNode *node = queuePool + topList[i];
        node->task.prevHits = node->task.hits - node->task.prevHits;
        node->task.prevTicks = node->task.ticks - node->task.prevTicks;
    }

    // sort active tasks
    for(int i = 0; i < cnt; i++) {
        for(int j = i + 1; j < cnt; j++) {
            int a = topList[i];
            int b = topList[j];
            if(queuePool[a].task.prevTicks < queuePool[b].task.prevTicks) {
                topList[i] = b;
                topList[j] = a;
            }
        }
    }

    // header row
    end = append(end, "  Call  Context  Hits     Micros  \n");

    uint32_t totalTicks = 0, totalHits = 0;
    for(int i = 0; i < cnt; i++) {
        QueueNode *node = queuePool + topList[i];

        if(node->task.run == doOnceExtended) {
            OnceExtended *ext = (OnceExtended *) node->task.ref;
            *(end++) = 'E';
            *(end++) = ' ';
            end += toHex((uint32_t) ext->run, 5, '0', end);
            *(end++) = ' ';
            end += toHex((uint32_t) ext->ref, 8, '0', end);
        } else {
            *(end++) = typeCode[node->task.type];
            *(end++) = ' ';
            end += toHex((uint32_t) node->task.run, 5, '0', end);
            *(end++) = ' ';
            end += toHex((uint32_t) node->task.ref, 8, '0', end);
        }
        *(end++) = ' ';
        end += fmtFloat(scale * (float) node->task.prevHits, 8, 0, end);
        *(end++) = ' ';
        end += fmtFloat(scale * 0.008f * (float) node->task.prevTicks, 8, 0, end);
        *(end++) = '\n';

        totalHits += node->task.prevHits;
        totalTicks += node->task.prevTicks;
    }

    *(end++) = '\n';
    end += padCopy(17, ' ', end, "Used ", 5);
    end += fmtFloat(scale * (float) totalHits, 8, 0, end);
    *(end++) = ' ';
    end += fmtFloat(scale * 0.008f * (float) totalTicks, 8, 0, end);
    *(end++) = '\n';

    end += padCopy(17, ' ', end, "Idle ", 5);
    end += fmtFloat(scale * (float) (queueSchedule.task.hits - idleHits), 8, 0, end);
    *(end++) = ' ';
    end += fmtFloat(scale * 0.008f * (float) (queueSchedule.task.ticks - idleTicks), 8, 0, end);
    *(end++) = '\n';

    idleHits = queueSchedule.task.hits;
    idleTicks = queueSchedule.task.ticks;

    for(int i = 0; i < cnt; i++) {
        QueueNode *node = queuePool + topList[i];
        node->task.prevHits = node->task.hits;
        node->task.prevTicks = node->task.ticks;
    }

    return end - buffer;
}
