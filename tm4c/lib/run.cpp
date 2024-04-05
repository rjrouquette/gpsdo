//
// Created by robert on 5/15/23.
//

#include "run.h"

#include "format.h"
#include "led.h"
#include "../hw/interrupts.h"
#include "clk/mono.h"

#include <memory>


inline uint32_t toMonoRaw(const uint32_t fixed_8_24) {
    uint64_t scratch = fixed_8_24;
    scratch *= CLK_FREQ;
    return scratch >> 24;
}

class Task {
    /**
     * Scheduling type
     */
    enum Schedule {
        Canceled,
        Sleep,
        Periodic
    };

    /**
     * Status type codes
     */
    static constexpr char typeCode[] = "CSP";

    /**
     * pointer to next task in the queue
     */
    Task *qNext;

    /**
     * pointer to previous task in the queue
     */
    Task *qPrev;

    /**
     * scheduling type
     */
    Schedule schedule;

    /**
     * the task callback
     */
    RunCall callback;

    /**
     * reference pointer for the task callback
     */
    void *reference;

    /**
     * The next scheduled run time.
     */
    uint32_t runNext;

    /**
     * The run or sleep interval.
     */
    uint32_t runInterval;

public:
    Task();

    /**
     * Allocate a new task.
     * @return the new task
     */
    static Task* alloc();

    /**
     * Deallocate the task.
     */
    void free();

    /**
     * Cancel the task.
     */
    void cancel();

    /**
     * Wake the task if it is sleeping.
     */
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

    /**
     * Get the next task in the queue.
     * @return the next task in the queue
     */
    [[nodiscard]]
    Task* next() const volatile {
        return qNext;
    }

    /**
     * Get the next task in the queue.
     * @return the next task in the queue
     */
    [[nodiscard]]
    Task* next() const {
        return qNext;
    }

    /**
     * Run the task.
     */
    void run() const {
        (*callback)(reference);
    }

    /**
     * Requeue the task.
     */
    void requeue();

    /**
     * Insert the task into the queue.
     */
    void insert();

    /**
     * Set the run or sleep interval.
     * @param interval the new run or sleep interval
     */
    void setInterval(const uint32_t interval) {
        runInterval = toMonoRaw(interval);
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
        return callback;
    }

    [[nodiscard]]
    void* getReference() const {
        return reference;
    }

    /**
     * Write task status to string buffer.
     * @param str string buffer
     * @return pointer to immediately after last character written
     */
    char* print(char *str) const;

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

    schedule = Canceled;
    callback = doNothing;
    reference = nullptr;

    runNext = 0;
    runInterval = 0;
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
    callback = doNothing;
    reference = nullptr;
    schedule = Canceled;
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
    auto ins = queueHead;
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

void Task::requeue() {
    if (schedule == Canceled) {
        // delete task it is canceled
        free();
        return;
    }

    // remove from queue
    pop();
    // set next run time
    runNext = runInterval + ((schedule == Periodic) ? runNext : CLK_MONO_RAW);
    // schedule next run
    insert();
}

void Task::setPeriodic(const uint32_t interval, const RunCall callback, void *ref) {
    schedule = Periodic;
    this->callback = callback;
    reference = ref;

    // convert fixed-point interval to raw monotonic domain
    runInterval = toMonoRaw(interval);
    // start after delay
    runNext = CLK_MONO_RAW + runInterval;
    // add to schedule
    insert();
}

void Task::setSleep(const uint32_t delay, const RunCall callback, void *ref) {
    schedule = Sleep;
    this->callback = callback;
    reference = ref;

    // convert fixed-point interval to raw monotonic domain
    runInterval = toMonoRaw(delay);
    // start after delay
    runNext = CLK_MONO_RAW + runInterval;
    // add to schedule
    insert();
}

char* Task::print(char *str) const {
    *str++ = typeCode[schedule];
    *str++ = ' ';
    str += toHex(reinterpret_cast<uint32_t>(callback), 5, '0', str);
    *str++ = ' ';
    str += toHex(reinterpret_cast<uint32_t>(reference), 8, '0', str);
    return str;
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
        task->requeue();
        __enable_irq();
    }
}

void* runSleep(uint32_t delay, RunCall callback, void *ref) {
    __disable_irq();
    const auto task = Task::alloc();
    task->setSleep(delay, callback, ref);
    __enable_irq();
    return task;
}

void* runPeriodic(uint32_t interval, RunCall callback, void *ref) {
    __disable_irq();
    const auto task = Task::alloc();
    task->setPeriodic(interval, callback, ref);
    __enable_irq();
    return task;
}

void runAdjust(void *taskHandle, const uint32_t interval) {
    static_cast<Task*>(taskHandle)->setInterval(interval);
}

void runWake(void *taskHandle) {
    __disable_irq();
    static_cast<Task*>(taskHandle)->wake();
    __enable_irq();
}

static void runCancel_(const RunCall callback, const void *const ref) {
    // match by reference
    if (callback == nullptr) {
        auto iter = queueHead;
        while (iter != queueRoot) {
            const auto task = iter;
            iter = iter->next();

            if (task->getReference() == ref)
                task->cancel();
        }
        return;
    }

    // match by callback
    if (ref == nullptr) {
        auto iter = queueHead;
        while (iter != queueRoot) {
            const auto task = iter;
            iter = iter->next();

            if (task->getCallback() == callback)
                task->cancel();
        }
        return;
    }

    // match both callback and reference
    auto iter = queueHead;
    while (iter != queueRoot) {
        const auto task = iter;
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
        end = task.print(end);
        *end++ = '\n';
    }
    return end - buffer;
}
