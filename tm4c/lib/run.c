//
// Created by robert on 5/15/23.
//

#include <stddef.h>
#include <memory.h>
#include "clk/mono.h"
#include "clk/util.h"
#include "format.h"
#include "led.h"
#include "run.h"


enum TaskType {
    TaskInvalid,
    TaskAlways,
    TaskSleep,
    TaskPeriodic,
    TaskOnce
};

typedef volatile struct QueueNode {
    // queue pointers
    volatile struct QueueNode *next;
    volatile struct QueueNode *prev;

    // task structure
    struct {
        enum TaskType type;
        SchedulerCallback callback;
        void *ref;
        uint32_t next;
        uint32_t intv;
        uint32_t hits;
        uint32_t ticks;
    } task;
} QueueNode;


#define SLOT_CNT (32)
static volatile QueueNode *queueFree;
static volatile QueueNode queuePool[SLOT_CNT];
static volatile QueueNode queueAlways;
static volatile QueueNode queueSchedule;

static QueueNode * allocNode();
static void freeNode(QueueNode *node);

static void queueInsBefore(QueueNode *ins, QueueNode *node);
static void queueRemove(QueueNode *node);

static void insSchedule(QueueNode  *node);

void initScheduler() {
    // initialize queue nodes
    queueFree = NULL;
    for(int i = 0; i < SLOT_CNT; i++)
        freeNode(queuePool + i);

    // initialize queue pointers
    queueAlways.next = &queueAlways;
    queueAlways.prev = &queueAlways;
    // initialize queue pointers
    queueSchedule.next = &queueSchedule;
    queueSchedule.prev = &queueSchedule;
}

static QueueNode * allocNode() {
    if(queueFree == NULL)
        faultBlink(3, 1);
    QueueNode *node = queueFree;
    queueFree = node->next;
    return node;
}

static void freeNode(QueueNode *node) {
    // clear node
    memset((void *) node, 0, sizeof(QueueNode));
    // push onto free stack
    node->next = queueFree;
    queueFree = node;
}

static void queueInsBefore(QueueNode *ins, QueueNode *node) {
    node->next = ins;
    node->prev = ins->prev;
    ins->prev = node;
    node->prev->next = node;
}

static void queueRemove(QueueNode *node) {
    if(node == node->next)
        faultBlink(3, 2);
    node->prev->next = node->next;
    node->next->prev = node->prev;
}

static void insSchedule(QueueNode *node) {
    // ordered insertion into schedule queue
    QueueNode *ins = queueSchedule.next;
    while(ins->task.type) {
        if(((int32_t) (node->task.next - ins->task.next)) < 0) {
            queueInsBefore(ins, node);
            return;
        }
        ins = ins->next;
    }
    // insert at end of queue if all other tasks in queue are scheduled sooner
    queueInsBefore(ins, node);
}

static void scheduleNext(QueueNode *node) {
    // remove from queue
    queueRemove(node);

    // compute next run time
    if(node->task.type == TaskPeriodic)
        node->task.next += node->task.intv;
    else if(node->task.type == TaskSleep)
        node->task.next = CLK_MONO_RAW + node->task.intv;
    else {
        // release node if task is complete
        freeNode(node);
        return;
    }

    // add to schedule
    insSchedule(node);
}

/**
 * Run task and update scheduling statistics
 * @param task
 */
static void runTask(QueueNode *node) {
    uint32_t start = CLK_MONO_RAW;
    (*(node->task.callback))(node->task.ref);
    node->task.ticks += CLK_MONO_RAW - start;
    ++(node->task.hits);
}

_Noreturn
void runScheduler() {
    QueueNode *next;

    // infinite loop, round-robin
    for (;;) {
        // check for always running tasks
        next = queueAlways.next;
        if(next->task.type) {
            runTask(next);
            // move task to end of queue if task was not cancelled
            if(next->task.type) {
                queueRemove(next);
                queueInsBefore(&queueAlways, next);
            }
        }

        // check for scheduled tasks
        next = queueSchedule.next;
        if(next->task.type) {
            // check clock
            if (((int32_t) (CLK_MONO_RAW - next->task.next)) >= 0) {
                // run the task
                runTask(next);
                // schedule next run if task was not cancelled
                if (next->task.type) {
                    scheduleNext(next);
                }
            }
        }
    }
}

void runAlways(SchedulerCallback callback, void *ref) {
    QueueNode *node = allocNode();
    node->task.type = TaskAlways;
    node->task.callback = callback;
    node->task.ref = ref;
    node->task.intv = 0;
    node->task.next = 0;
    queueInsBefore(&queueAlways, node);
}

void runSleep(uint64_t delay, SchedulerCallback callback, void *ref) {
    QueueNode *node = allocNode();
    node->task.type = TaskSleep;
    node->task.callback = callback;
    node->task.ref = ref;

    // convert fixed-point interval to raw monotonic domain
    union fixed_32_32 scratch;
    scratch.full = delay;
    scratch.full *= CLK_FREQ;
    node->task.intv = scratch.ipart;

    // start immediately
    node->task.next = CLK_MONO_RAW;
    // add to schedule
    insSchedule(node);
}

void runPeriodic(uint64_t interval, SchedulerCallback callback, void *ref) {
    QueueNode *node = allocNode();
    node->task.type = TaskPeriodic;
    node->task.callback = callback;
    node->task.ref = ref;

    // convert fixed-point interval to raw monotonic domain
    union fixed_32_32 scratch;
    scratch.full = interval;
    scratch.full *= CLK_FREQ;
    node->task.intv = scratch.ipart;

    // start immediately
    node->task.next = CLK_MONO_RAW;
    // add to schedule
    insSchedule(node);
}

void runOnce(uint64_t delay, SchedulerCallback callback, void *ref) {
    QueueNode *node = allocNode();
    node->task.type = TaskOnce;
    node->task.callback = callback;
    node->task.ref = ref;
    node->task.intv = 0;

    // convert fixed-point interval to raw monotonic domain
    union fixed_32_32 scratch;
    scratch.full = delay;
    scratch.full *= CLK_FREQ;

    // start after delay
    node->task.next = CLK_MONO_RAW + scratch.ipart;
    // add to schedule
    insSchedule(node);
}

static void purgeFromQueue(QueueNode *next, SchedulerCallback callback, void *ref) {
    while(next->task.type) {
        QueueNode *node = next;
        next = next->next;

        if(node->task.callback == callback) {
            if((ref == NULL) || (node->task.ref == ref)) {
                queueRemove(node);
                freeNode(node);
            }
        }
    }
}

void runCancel(SchedulerCallback callback, void *ref) {
    purgeFromQueue(queueAlways.next, callback, ref);
    purgeFromQueue(queueSchedule.next, callback, ref);
}


static volatile uint32_t prevQuery;
static volatile uint32_t prevHits[SLOT_CNT];
static volatile uint32_t prevTicks[SLOT_CNT];

static const char typeCode[5] = {
        'D', 'A', 'S', 'P', 'O'
};

static char * writeNode(char *ptr, float scale, uint32_t *totalTicks, QueueNode *node) {
    int i = node - queuePool;

    uint32_t ticks = node->task.ticks - prevTicks[i];
    prevTicks[i] += ticks;

    uint32_t hits = node->task.hits - prevHits[i];
    prevHits[i] += hits;

    *totalTicks += ticks;

    *(ptr++) = typeCode[node->task.type];
    *(ptr++) = ' ';
    ptr += toHex((uint32_t) node->task.callback, 6, '0', ptr);
    *(ptr++) = ' ';
    ptr += toHex((uint32_t) node->task.ref, 8, '0', ptr);
    *(ptr++) = ' ';
    ptr += fmtFloat(scale * (float) hits, 8, 1, ptr);
    *(ptr++) = ' ';
    ptr += fmtFloat(scale * 0.008f * (float) ticks, 8, 1, ptr);
    *(ptr++) = '\n';

    return ptr;
}

unsigned runStatus(char *buffer) {
    char *end = buffer;

    uint32_t elapsed = CLK_MONO_RAW - prevQuery;
    prevQuery += elapsed;
    float scale = 125e6f / (float) elapsed;

    uint32_t total = 0;

    QueueNode *next = queueAlways.next;
    while(next->task.type) {
        QueueNode *node = next;
        next = next->next;
        end = writeNode(end, scale, &total, node);
    }

    next = queueSchedule.next;
    while(next->task.type) {
        QueueNode *node = next;
        next = next->next;
        end = writeNode(end, scale, &total, node);
    }

    end += fmtFloat(scale * 0.008f * (float) total, 35, 1, end);
    *(end++) = '\n';

    return end - buffer;
}
