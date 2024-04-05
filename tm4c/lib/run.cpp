//
// Created by robert on 5/15/23.
//

#include "run.h"

#include "format.h"
#include "led.h"
#include "../hw/interrupts.h"
#include "clk/mono.h"

#include <cstddef>
#include <memory>


inline uint32_t toMonoRaw(const uint32_t fixed_8_24) {
    uint64_t scratch = fixed_8_24;
    scratch *= CLK_FREQ;
    return scratch >> 24;
}

enum RunType {
    RunCanceled,
    RunSleep,
    RunPeriodic
};

class Task {
    static constexpr char typeCode[] = "CSP";

    // queue pointers
    Task *qNext;
    Task *qPrev;

    // task fields
    RunType runType;
    RunCall runCall;
    void *runRef;
    uint32_t runNext;
    uint32_t runIntv;

public:
    Task();

    static Task* alloc();

    void free();

    void cancel();

    void wake();

    /**
     * Determine if the task is ready to run.
     * @return true if the task is ready to run
     */
    [[nodiscard]]
    bool isReady() const {
        return static_cast<int32_t>(CLK_MONO_RAW - runNext) >= 0;
    }

    /**
     * Remove the task from the queue.
     */
    void pop() {
        qPrev->qNext = qNext;
        qNext->qPrev = qPrev;
        qNext = nullptr;
        qPrev = nullptr;
    }

    [[nodiscard]]
    Task* next() const volatile {
        return qNext;
    }

    [[nodiscard]]
    Task* next() const {
        return qNext;
    }

    /**
     * Run the task.
     */
    void run() const {
        (*runCall)(runRef);
    }

    void reschedule();

    void insert();

    void setInterval(const uint32_t interval) {
        runIntv = toMonoRaw(interval);
    }

    /**
     * Schedule task to execute at a regular interval (exact execution rate)
     * @param interval interval in 8.24 fixed point format (16 second maximum, uses monotonic clock)
     * @param callback task entry point
     * @param ref context pointer for task
     * @return task handle
     */
    void setPeriodic(uint32_t interval, RunCall callback, void *ref);

    /**
     * Schedule task to sleep for a fixed delay between executions
     * @param delay interval in 8.24 fixed point format (16 second maximum, uses monotonic clock)
     * @param callback task entry point
     * @param ref context pointer for task
     * @return task handle
     */
    void setSleep(uint32_t delay, RunCall callback, void *ref);

    [[nodiscard]]
    RunCall getCallback() const {
        return runCall;
    }

    [[nodiscard]]
    void* getReference() const {
        return runRef;
    }

    void print(char *&ptr) const;

    friend void initScheduler();
};

#define SLOT_CNT (32)
static Task *taskFree;
static Task taskPool[SLOT_CNT];
static volatile Task taskQueue;
#define queueHead (taskQueue.next())
#define queueRoot ((Task *) &taskQueue)

static void doNothing(void *ref) {}

void initScheduler() {
    // initialize queue nodes
    taskFree = taskPool;
    for (int i = 1; i < SLOT_CNT; i++)
        taskPool[i - 1].qNext = taskPool + i;

    // initialize queue pointers
    taskQueue.qNext = queueRoot;
    taskQueue.qPrev = queueRoot;
}

Task::Task() {
    qNext = nullptr;
    qPrev = nullptr;

    runType = RunCanceled;
    runCall = doNothing;
    runRef = nullptr;

    runNext = 0;
    runIntv = 0;
}

Task* Task::alloc() {
    if (taskFree == nullptr)
        faultBlink(3, 1);
    const auto task = taskFree;
    taskFree = task->qNext;
    return new(task) Task();
}

void Task::free() {
    pop();
    // push onto free stack
    qNext = taskFree;
    taskFree = this;
}

void Task::cancel() {
    // cancel task
    runCall = doNothing;
    runRef = nullptr;
    runType = RunCanceled;
    // explicitly delete task if it is not currently running
    if (this != queueHead)
        free();
}

void Task::wake() {
    // remove from queue
    pop();
    // set to run immediately
    runNext = CLK_MONO_RAW;
    // update schedule queue
    insert();
}

void Task::insert() {
    // locate optimal insertion point
    Task *ins = queueHead;
    while (ins != queueRoot) {
        if (static_cast<int32_t>(runNext - ins->runNext) < 0)
            break;
        ins = ins->qNext;
    }

    // insert task into the scheduling queue
    qNext = ins;
    qPrev = ins->qPrev;
    ins->qPrev = this;
    qPrev->qNext = this;
}

void Task::reschedule() {
    if (runType == RunCanceled) {
        // delete task it is canceled
        free();
        return;
    }

    // remove from queue
    pop();
    // set next run time
    runNext = runIntv + ((runType == RunPeriodic) ? runNext : CLK_MONO_RAW);
    // schedule next run
    insert();
}

void Task::setPeriodic(uint32_t interval, RunCall callback, void *ref) {
    runType = RunPeriodic;
    runCall = callback;
    runRef = ref;

    // convert fixed-point interval to raw monotonic domain
    runIntv = toMonoRaw(interval);
    // start after delay
    runNext = CLK_MONO_RAW + runIntv;
    // add to schedule
    insert();
}

void Task::setSleep(uint32_t delay, RunCall callback, void *ref) {
    runType = RunSleep;
    runCall = callback;
    runRef = ref;

    // convert fixed-point interval to raw monotonic domain
    runIntv = toMonoRaw(delay);
    // start after delay
    runNext = CLK_MONO_RAW + runIntv;
    // add to schedule
    insert();
}

void Task::print(char *&ptr) const {
    *ptr++ = typeCode[runType];
    *ptr++ = ' ';
    ptr += toHex(reinterpret_cast<uint32_t>(runCall), 5, '0', ptr);
    *ptr++ = ' ';
    ptr += toHex(reinterpret_cast<uint32_t>(runRef), 8, '0', ptr);
}


[[noreturn]]
void runScheduler() {
    // infinite loop
    for (;;) {
        // check for scheduled tasks
        const auto task = queueHead;
        if (!task->isReady())
            continue;

        // run the task
        task->run();

        // determine next state
        __disable_irq();
        task->reschedule();
        __enable_irq();
    }
}

void* runSleep(uint32_t delay, RunCall callback, void *ref) {
    __disable_irq();
    Task *task = Task::alloc();
    task->setSleep(delay, callback, ref);
    __enable_irq();
    return task;
}

void* runPeriodic(uint32_t interval, RunCall callback, void *ref) {
    __disable_irq();
    Task *task = Task::alloc();
    task->setPeriodic(interval, callback, ref);
    __enable_irq();
    return task;
}

void runAdjust(void *taskHandle, uint32_t newInterval) {
    // set new run interval
    static_cast<Task*>(taskHandle)->setInterval(newInterval);
}

void runWake(void *taskHandle) {
    __disable_irq();
    static_cast<Task*>(taskHandle)->wake();
    __enable_irq();
}

static void runCancel_(const RunCall callback, const void *const ref) {
    // match by reference
    if (callback == nullptr) {
        Task *iter = queueHead;
        while (iter != queueRoot) {
            Task *task = iter;
            iter = iter->next();

            if (task->getReference() == ref)
                task->cancel();
        }
        return;
    }

    // match by callback
    if (ref == nullptr) {
        Task *iter = queueHead;
        while (iter != queueRoot) {
            Task *task = iter;
            iter = iter->next();

            if (task->getCallback() == callback)
                task->cancel();
        }
        return;
    }

    // match both callback and reference
    Task *iter = queueHead;
    while (iter != queueRoot) {
        Task *task = iter;
        iter = iter->next();

        if (task->getCallback() == callback && task->getReference() == ref)
            task->cancel();
    }
}

void runCancel(RunCall callback, void *ref) {
    __disable_irq();
    runCancel_(callback, ref);
    __enable_irq();
}

unsigned runStatus(char *buffer) {
    char *end = buffer;

    // header row
    end = append(end, "  Call  Context\n");

    for (const auto &task : taskPool) {
        task.print(end);
        *end++ = '\n';
    }
    return end - buffer;
}
