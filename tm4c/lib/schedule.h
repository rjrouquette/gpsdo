//
// Created by robert on 5/15/23.
//

#ifndef GPSDO_LIB_SCHEDULE_H
#define GPSDO_LIB_SCHEDULE_H

#include <stdint.h>

typedef void (*SchedulerCallback)(void *ref);

/**
 * Run scheduler (infinite loop)
 */
_Noreturn
void runScheduler();

/**
 * Schedule task for continuous polling
 * @param callback task entry point
 * @param ref context pointer for task
 */
void runAlways(SchedulerCallback callback, void *ref);

/**
 * Schedule task to execute at a regular interval
 * @param interval interval in 32.32 fixed point format (monotonic clock)
 * @param callback task entry point
 * @param ref context pointer for task
 */
void runInterval(uint64_t interval, SchedulerCallback callback, void *ref);

/**
 * Schedule task to execute at a specific time in the future.  The task is removed from the queue when run.
 * @param when timestamp in 32.32 fixed point format (monotonic clock)
 * @param callback task entry point
 * @param ref context pointer for task
 */
void runOnce(uint64_t when, SchedulerCallback callback, void *ref);

/**
 * Remove task with associated callback and reference pointer from the scheduler queue
 * @param callback task entry point
 * @param ref context pointer for task (if NULL, all entries matching callback are removed)
 */
void runRemove(SchedulerCallback callback, void *ref);

/**
 * Write current status of the scheduler to a buffer
 * @param buffer destination for status information
 * @return number of bytes written to buffer
 */
unsigned runStatus(char *buffer);

#endif //GPSDO_LIB_SCHEDULE_H
