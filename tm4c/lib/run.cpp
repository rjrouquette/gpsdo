//
// Created by robert on 5/15/23.
//

#include "run.h"

#include "format.h"
#include "led.h"
#include "clk/mono.h"

#include <memory>


class Task {
    /**
     * Scheduling type
     */
    enum Schedule {
        Free,
        Canceled,
        Sleep,
        Periodic,
        Wait,
        Wake
    };

    /**
     * Status type codes
     */
    static constexpr char typeCode[] = "-CSPWQ";

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

    /**
     * Task callback which does nothing. Used for task cancellation.
     * @param ref task reference pointer
     */
    static void doNothing([[maybe_unused]] void *ref) {}

    /**
     * Convert 8.24 fixed-point seconds into raw timer ticks.
     * @param fixed_8_24 fixed-point value to convert
     * @return number of timer ticks equivalent to the fixed-point value
     */
    static uint32_t toMonoRaw(const uint32_t fixed_8_24) {
        uint64_t scratch = fixed_8_24;
        scratch *= CLK_FREQ;
        return scratch >> 24;
    }

public:
    Task();

    /**
     * Allocate a new task.
     * @return the new task
     */
    static Task* alloc();

    /**
     * Cancel the task.
     */
    void cancel() {
        callback = doNothing;
        reference = nullptr;
        schedule = Canceled;
    }

    /**
     * Mark task as awoken.
     */
    void wake() {
        schedule = Wake;
    }

    /**
     * Determine if the task is ready to run.
     * @return true if the task is ready to run
     */
    [[nodiscard]]
    bool isReady() const {
        return static_cast<int32_t>(CLK_MONO_RAW - runNext) >= 0;
    }

    /**
     * Determine if the task has been freed.
     * @return true if the task has been freed
     */
    [[nodiscard]]
    bool isFree() const {
        return schedule == Free;
    }

    /**
     * Determine if the task has been canceled.
     * @return true if the task has been canceled
     */
    [[nodiscard]]
    bool isCanceled() const {
        return callback == doNothing;
    }

    /**
     * Remove the task from the queue.
     */
    void pop() const {
        qPrev->qNext = qNext;
        qNext->qPrev = qPrev;
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
    void run() {
        if (schedule == Wait)
            return;
        // if thread was awoken, set it back to waiting
        if (schedule == Wake)
            schedule = Wait;
        // perform task
        (*callback)(reference);
    }

    /**
     * Process out-of-band updates.
     */
    void update();

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

    /**
     * Schedule task to wait for calls to runWake().
     * @param callback task entry point
     * @param ref context pointer for task
     * @return task handle
     */
    void setWait(RunCall callback, void *ref);

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
static volatile bool taskUpdate;
#define queueHead (taskQueue.next())
#define queueRoot ((Task *) &taskQueue)

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

void Task::update() {
    // free task if it has been canceled
    if (callback == doNothing) {
        pop();
        // push onto free stack
        callback = nullptr;
        schedule = Free;
        qPrev = nullptr;
        qNext = taskFree;
        taskFree = this;
        return;
    }

    // wake task
    if (schedule == Wake) {
        // remove from queue
        pop();
        // set to run immediately
        runNext = CLK_MONO_RAW;
        // update schedule queue
        insert();
    }
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
    if (schedule == Canceled)
        return;

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

void Task::setWait(const RunCall callback, void *ref) {
    schedule = Sleep;
    this->callback = callback;
    reference = ref;

    // set run interval to a reasonably long value (without wrapping)
    runInterval = 1u << 30;
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
        // check for out-of-band updates
        if (taskUpdate) {
            taskUpdate = false;
            for (auto &task : taskPool)
                task.update();
        }

        // check for scheduled tasks
        const auto task = queueHead;
        if (!task->isReady())
            continue;

        // run the task
        task->run();
        // requeue the task
        task->requeue();
    }
}

void* runWait(RunCall callback, void *ref) {
    const auto task = Task::alloc();
    task->setWait(callback, ref);
    return task;
}

void* runSleep(uint32_t delay, RunCall callback, void *ref) {
    const auto task = Task::alloc();
    task->setSleep(delay, callback, ref);
    return task;
}

void* runPeriodic(uint32_t interval, RunCall callback, void *ref) {
    const auto task = Task::alloc();
    task->setPeriodic(interval, callback, ref);
    return task;
}

void runAdjust(void *taskHandle, const uint32_t interval) {
    static_cast<Task*>(taskHandle)->setInterval(interval);
}

void runWake(void *taskHandle) {
    const auto task = static_cast<Task*>(taskHandle);
    if (task->isFree() || task->isCanceled())
        return;
    task->wake();
    taskUpdate = true;
}

static void runCancel(void *taskHandle) {
    static_cast<Task*>(taskHandle)->cancel();
    taskUpdate = true;
}

void runCancel(const RunCall callback, const void *ref) {
    // match by reference
    if (callback == nullptr) {
        auto iter = queueHead;
        while (iter != queueRoot) {
            const auto task = iter;
            iter = iter->next();

            if (task->getReference() == ref)
                runCancel(task);
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
                runCancel(task);
        }
        return;
    }

    // match both callback and reference
    auto iter = queueHead;
    while (iter != queueRoot) {
        const auto task = iter;
        iter = iter->next();

        if (task->getCallback() == callback && task->getReference() == ref)
            runCancel(task);
    }
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
