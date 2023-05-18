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
        SchedulerCallback run;
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
static volatile QueueNode queueSchedule;

static QueueNode * allocNode();
static void freeNode(QueueNode *node);


void initScheduler() {
    // initialize queue nodes
    queueFree = NULL;
    for(int i = 0; i < SLOT_CNT; i++)
        freeNode(queuePool + i);

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

__attribute__((always_inline))
static inline void queueRemove(QueueNode *node) {
    node->prev->next = node->next;
    node->next->prev = node->prev;
}

static void insSchedule(QueueNode *node) {
    // ordered insertion into schedule queue
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
}

__attribute__((optimize(3)))
_Noreturn
void runScheduler() {
    QueueNode *node = &queueSchedule;
    uint32_t now = CLK_MONO_RAW;

    // infinite loop
    for (;;) {
        // record time spent on prior task
        uint32_t prior = now;
        now = CLK_MONO_RAW;
        ++(node->task.hits);
        node->task.ticks += now - prior;

        // check for scheduled tasks
        node = queueSchedule.next;
        if(((int32_t) (now - node->task.next)) < 0) {
            // credit time spent to the scheduler
            node = &queueSchedule;
            continue;
        }

        // run the task
        (*(node->task.run))(node->task.ref);
        // remove from queue
        queueRemove(node);
        // compute next run time
        if(node->task.type == TaskSleep)
            node->task.next = CLK_MONO_RAW + node->task.intv;
        else if(node->task.type == TaskPeriodic)
            node->task.next += node->task.intv;
        else {
            // release node if task is complete
            freeNode(node);
            // credit time spent to the scheduler
            node = &queueSchedule;
            continue;
        }
        // add to schedule
        insSchedule(node);
    }
}

void runSleep(uint64_t delay, SchedulerCallback callback, void *ref) {
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
    insSchedule(node);
}

void runPeriodic(uint64_t interval, SchedulerCallback callback, void *ref) {
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
    insSchedule(node);
}

void runOnce(uint64_t delay, SchedulerCallback callback, void *ref) {
    QueueNode *node = allocNode();
    node->task.type = TaskOnce;
    node->task.run = callback;
    node->task.ref = ref;

    // convert fixed-point interval to raw monotonic domain
    union fixed_32_32 scratch;
    scratch.full = delay;
    scratch.full *= CLK_FREQ;
    node->task.intv = scratch.ipart;

    // start after delay
    node->task.next = CLK_MONO_RAW + scratch.ipart;
    // add to schedule
    insSchedule(node);
}

void runCancel(SchedulerCallback callback, void *ref) {
    QueueNode *next = queueSchedule.next;
    while(next->task.type) {
        QueueNode *node = next;
        next = next->next;

        if(node->task.run == callback) {
            if((ref == NULL) || (node->task.ref == ref)) {
                // check if the currently running task is being cancelled
                if(node == queueSchedule.next) {
                    // if so, defer cleanup to main loop
                    node->task.type = TaskOnce;
                } else {
                    // if not, explicitly cancel task and free memory
                    queueRemove(node);
                    freeNode(node);
                }
            }
        }
    }
}


static volatile uint32_t prevQuery, idleHits, idleTicks;
static volatile uint32_t prevHits[SLOT_CNT];
static volatile uint32_t prevTicks[SLOT_CNT];
static volatile int topList[SLOT_CNT];

static const char typeCode[5] = {
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
        int j = topList[i];
        prevHits[j] = queuePool[j].task.hits - prevHits[j];
        prevTicks[j] = queuePool[j].task.ticks - prevTicks[j];
    }

    // sort active tasks
    for(int i = 0; i < cnt; i++) {
        for(int j = i + 1; j < cnt; j++) {
            int a = topList[i];
            int b = topList[j];
            if(prevTicks[a] < prevTicks[b]) {
                topList[i] = b;
                topList[j] = a;
            }
        }
    }

    // header row
    end = append(end, "  Call   Context  Hits     Micros  \n");

    uint32_t totalTicks = 0, totalHits = 0;
    for(int i = 0; i < cnt; i++) {
        int j = topList[i];
        QueueNode *node = queuePool + j;
        
        *(end++) = typeCode[node->task.type];
        *(end++) = ' ';
        end += toHex((uint32_t) node->task.run, 6, '0', end);
        *(end++) = ' ';
        end += toHex((uint32_t) node->task.ref, 8, '0', end);
        *(end++) = ' ';
        end += fmtFloat(scale * (float) prevHits[j], 8, 0, end);
        *(end++) = ' ';
        end += fmtFloat(scale * 0.008f * (float) prevTicks[j], 8, 0, end);
        *(end++) = '\n';

        totalHits += prevHits[j];
        totalTicks += prevTicks[j];
    }

    *(end++) = '\n';
    end += padCopy(18, ' ', end, "Used ", 5);
    end += fmtFloat(scale * (float) totalHits, 8, 0, end);
    *(end++) = ' ';
    end += fmtFloat(scale * 0.008f * (float) totalTicks, 8, 0, end);
    *(end++) = '\n';

    end += padCopy(18, ' ', end, "Idle ", 5);
    end += fmtFloat(scale * (float) (queueSchedule.task.hits - idleHits), 8, 0, end);
    *(end++) = ' ';
    end += fmtFloat(scale * 0.008f * (float) (queueSchedule.task.ticks - idleTicks), 8, 0, end);
    *(end++) = '\n';

    idleHits = queueSchedule.task.hits;
    idleTicks = queueSchedule.task.ticks;

    for(int i = 0; i < cnt; i++) {
        int j = topList[i];
        prevHits[j] = queuePool[j].task.hits;
        prevTicks[j] = queuePool[j].task.ticks;
    }

    return end - buffer;
}
