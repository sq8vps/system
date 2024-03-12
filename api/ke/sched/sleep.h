//This header file is generated automatically
#ifndef EXPORTED___API__KE_SCHED_SLEEP_H_
#define EXPORTED___API__KE_SCHED_SLEEP_H_

#ifdef __cplusplus
extern "C" 
{
#endif

#include <stdint.h>
#include "defines.h"
struct KeTaskControlBlock;

/**
 * @brief Put arbitrary task to sleep for a given time
 * @param *tcb Task Control Block
 * @param time Time in nanoseconds
 * @return Status code
 * @warning The sleep is guaranteed to be not shorter than \a time, but the actual sleep time depends on how tasks are scheduled
*/
extern STATUS KePutTaskToSleep(struct KeTaskControlBlock *tcb, uint64_t time);

/**
 * @brief Put current task to sleep for a given time
 * @param time Time in nanoseconds
 * @return Status code
 * @warning The sleep is guaranteed to be not shorter than \a time, but the actual sleep time depends on how tasks are scheduled
*/
extern STATUS KeSleep(uint64_t time);

/**
 * @brief Perform a blocking delay
 * @param time Time in nanoseconds
*/
extern void KeDelay(uint64_t time);


#ifdef __cplusplus
}
#endif

#endif