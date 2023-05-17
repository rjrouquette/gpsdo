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
 * Schedule task for continuous polling
 * @param callback task entry point
 * @param ref context pointer for task
 */
void runAlways(SchedulerCallback callback, void *ref);

/**
 * Schedule task to sleep for a fixed delay between executions
 * @param delay interval in 32.32 fixed point format (16 second maximum, uses monotonic clock)
 * @param callback task entry point
 * @param ref context pointer for task
 */
void runSleep(uint64_t delay, SchedulerCallback callback, void *ref);

/**
 * Schedule task to execute at a regular interval (exact execution rate)
 * @param interval interval in 32.32 fixed point format (16 second maximum, uses monotonic clock)
 * @param callback task entry point
 * @param ref context pointer for task
 */
void runPeriodic(uint64_t interval, SchedulerCallback callback, void *ref);

/**
 * Schedule task to execute after a delay
 * @param delay delay in 32.32 fixed point format (16 second maximum, uses monotonic clock)
 * @param callback task entry point
 * @param ref context pointer for task
 */
void runLater(uint64_t delay, SchedulerCallback callback, void *ref);

/**
 * Cancel execution of task with associated callback and reference pointer from the scheduler queue
 * @param callback task entry point
 * @param ref context pointer for task (if NULL, all entries matching callback are removed)
 */
void runCancel(SchedulerCallback callback, void *ref);

/**
 * Write current status of the scheduler to a buffer
 * @param buffer destination for status information
 * @return number of bytes written to buffer
 */
unsigned runStatus(char *buffer);

#endif //GPSDO_LIB_RUN_H
