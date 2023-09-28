//This header file is generated automatically
#ifndef EXPORTED___API__KE_SCHED_SCHED_H_
#define EXPORTED___API__KE_SCHED_SCHED_H_

#ifdef __cplusplus
extern "C" 
{
#endif

#include <stdint.h>
#include "defines.h"
#include "ke/task/task.h"
/**
 * @brief Change task major priority/scheduling policy
 * @param *tcb Task Control Block pointer
 * @param priority New priority level
 * @return Status code
*/
extern STATUS KeChangeTaskMajorPriority(struct KeTaskControlBlock *tcb, enum KeTaskMajorPriority priority);

/**
 * @brief Change task minor priority/scheduling policy
 * @param *tcb Task Control Block pointer
 * @param priority New priority level
 * @return Status code
*/
extern STATUS KeChangeTaskMinorPriority(struct KeTaskControlBlock *tcb, uint8_t priority);

/**
 * @brief Enable task for scheduling
 * @param *tcb Task Control Block
 * @return Status code
*/
extern STATUS KeEnableTask(struct KeTaskControlBlock *tcb);

/**
 * @brief Suspend task and yield to scheduler
 * @attention Program execution continues from this point
*/
extern void KeTaskYield(void);

/**
 * @brief Get current task pointer
 * @return Current Task Control Block
*/
extern struct KeTaskControlBlock* KeGetCurrentTask(void);


#ifdef __cplusplus
}
#endif

#endif