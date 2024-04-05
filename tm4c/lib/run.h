//
// Created by robert on 5/15/23.
//

#ifndef GPSDO_LIB_RUN_H
#define GPSDO_LIB_RUN_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define RUN_MAX (1u << 28)
#define RUN_SEC (1u << 24)

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
#ifdef __cplusplus
[[noreturn]]
#else
_Noreturn
#endif
void runScheduler();

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
 * Wake a sleeping task
 * @param taskHandle the sleeping task to wake
 */
void runWake(void *taskHandle);

/**
 * Cancel execution of task with associated callback and reference pointer from the scheduler queue
 * @param callback task entry point
 * @param ref context pointer for task (if NULL, all entries matching callback are removed)
 */
void runCancel(RunCall callback, void *ref);

/**
 * Write current status of the scheduler to a buffer
 * @param buffer destination for status information
 * @return number of bytes written to buffer
 */
unsigned runStatus(char *buffer);

#ifdef __cplusplus
}
#endif

#endif //GPSDO_LIB_RUN_H
