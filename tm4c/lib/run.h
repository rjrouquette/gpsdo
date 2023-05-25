//
// Created by robert on 5/15/23.
//

#ifndef GPSDO_LIB_RUN_H
#define GPSDO_LIB_RUN_H

#include <stdint.h>

typedef void (*SchedulerCallback)(void *ref);

/**
 * Initialize scheduler
 */
void initScheduler();

/**
 * Run scheduler (infinite loop)
 */
_Noreturn
void runScheduler();

/**
 * Schedule task to sleep for a fixed delay between executions
 * @param delay interval in 32.32 fixed point format (16 second maximum, uses monotonic clock)
 * @param callback task entry point
 * @param ref context pointer for task
 * @return task handle
 */
void * runSleep(uint64_t delay, SchedulerCallback callback, void *ref);

/**
 * Schedule task to execute at a regular interval (exact execution rate)
 * @param interval interval in 32.32 fixed point format (16 second maximum, uses monotonic clock)
 * @param callback task entry point
 * @param ref context pointer for task
 * @return task handle
 */
void * runPeriodic(uint64_t interval, SchedulerCallback callback, void *ref);

/**
 * Schedule task to execute after a delay
 * @param delay delay in 32.32 fixed point format (16 second maximum, uses monotonic clock)
 * @param callback task entry point
 * @param ref context pointer for task
 * @return task handle
 */
void * runOnce(uint64_t delay, SchedulerCallback callback, void *ref);

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
void runCancel(SchedulerCallback callback, void *ref);

/**
 * Cancel execution of specific task and release its scheduling resources
 * @param taskHandle task to cancel
 */
void runRemove(void *taskHandle);

/**
 * Write current status of the scheduler to a buffer
 * @param buffer destination for status information
 * @return number of bytes written to buffer
 */
unsigned runStatus(char *buffer);

#endif //GPSDO_LIB_RUN_H
