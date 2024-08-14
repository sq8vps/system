#ifndef KERNEL_SLEEP_H_
#define KERNEL_SLEEP_H_

#include <stdint.h>
#include "defines.h"

EXPORT_API

struct KeTaskControlBlock;


/**
 * @brief Put arbitrary task to sleep for a given time
 * @param *tcb Task Control Block
 * @param time Time in nanoseconds
 * @return Status code
 * @warning The sleep is guaranteed to be not shorter than \a time, but the actual sleep time depends on how tasks are scheduled
*/
STATUS KePutTaskToSleep(struct KeTaskControlBlock *tcb, uint64_t time);


/**
 * @brief Put current task to sleep for a given time
 * @param time Time in nanoseconds
 * @return Status code
 * @warning The sleep is guaranteed to be not shorter than \a time, but the actual sleep time depends on how tasks are scheduled
*/
STATUS KeSleep(uint64_t time);


/**
 * @brief Perform a blocking delay
 * @param time Time in nanoseconds
*/
void KeDelay(uint64_t time);

END_EXPORT_API

/**
 * @brief Refresh sleeping task list - check and wake appropriate tasks
 * @return Status code
*/
INTERNAL STATUS KeRefreshSleepingTasks(void);

#endif