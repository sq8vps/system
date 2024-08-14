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
STATUS KeChangeTaskMajorPriority(struct KeTaskControlBlock *tcb, enum KeTaskMajorPriority priority);


/**
 * @brief Change task minor priority/scheduling policy
 * @param *tcb Task Control Block pointer
 * @param priority New priority level
 * @return Status code
*/
STATUS KeChangeTaskMinorPriority(struct KeTaskControlBlock *tcb, uint8_t priority);


/**
 * @brief Enable task for scheduling
 * @param *tcb Task Control Block
 * @return Status code
*/
STATUS KeEnableTask(struct KeTaskControlBlock *tcb);


/**
 * @brief Suspend task and yield to scheduler
 * @attention Program execution continues from this point
*/
void KeTaskYield(void);

/**
 * @brief Block task (remove from ready-to-run queue)
 * @param *tcb Task Control Block
 * @param reason Reason for task block
 */
void KeBlockTask(struct KeTaskControlBlock *tcb, enum KeTaskBlockReason reason);

/**
 * @brief Unblock task (insert to ready-to-run queue)
 * @param *tcb Task Control Block
*/
void KeUnblockTask(struct KeTaskControlBlock *tcb);

/**
 * @brief Get task block state
 * @param *tcb Task Control Block
 * @return Block state/block reason
 */
enum KeTaskBlockReason KeGetTaskBlockState(struct KeTaskControlBlock *tcb);


/**
 * @brief Get current task pointer
 * @return Current Task Control Block
*/
struct KeTaskControlBlock* KeGetCurrentTask(void);


#ifdef __cplusplus
}
#endif

#endif