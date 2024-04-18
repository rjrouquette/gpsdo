//
// Created by robert on 5/15/23.
//

#pragma once

#include <cstdint>
#include "clock/clock.hpp"
#include "clock/mono.hpp"

static constexpr uint32_t RUN_SEC = 1u << 24;
static constexpr uint32_t RUN_MAX = static_cast<uint64_t>(MAX_RAW_INTV) * RUN_SEC / CLK_FREQ;

/**
 * Callback function typedef
 */
typedef void (*RunCall)(void *ref);

/**
 * Initialize scheduler
 */
void initScheduler();

/**
 * Run scheduler (infinite loop)
*/
[[noreturn]]
void runScheduler();

/**
 * Schedule task to wait for calls to runWake().
 * @param callback task entry point
 * @param ref context pointer for task
 * @return task handle
 */
void* runWait(RunCall callback, void *ref);

/**
 * Schedule task to sleep for a fixed delay between executions
 * @param delay interval in 8.24 fixed point format (16 second maximum, uses monotonic clock)
 * @param callback task entry point
 * @param ref context pointer for task
 * @return task handle
 */
void* runSleep(uint32_t delay, RunCall callback, void *ref);

/**
 * Schedule task to execute at a regular interval (exact execution rate)
 * @param interval interval in 8.24 fixed point format (16 second maximum, uses monotonic clock)
 * @param callback task entry point
 * @param ref context pointer for task
 * @return task handle
 */
void* runPeriodic(uint32_t interval, RunCall callback, void *ref);

/**
 * Adjust the execution interval for a task
 * @param taskHandle the task to adjust
 * @param interval new interval in 8.24 fixed point format (16 second maximum, uses monotonic clock)
 */
void runAdjust(void *taskHandle, uint32_t interval);

/**
 * Wake a waiting task
 * @param taskHandle the waiting task to wake
 */
void runWake(void *taskHandle);

/**
 * Cancel execution of task with associated callback and reference pointer from the scheduler queue
 * @param callback task entry point
 * @param ref context pointer for task (if NULL, all entries matching callback are removed)
 */
void runCancel(RunCall callback, const void *ref);

/**
 * Write current status of the scheduler to a buffer
 * @param buffer destination for status information
 * @return number of bytes written to buffer
 */
unsigned runStatus(char *buffer);
