#ifndef KERNEL_SCHED_H_
#define KERNEL_SCHED_H_

#include <stdint.h>
#include "defines.h"
#include "ke/task/task.h"

EXPORT_API

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

/**
 * @brief Get current task's parent (if has a parent) or self (if doesn't have a parent) pointer
 * @return Current task's parent Task Control Block
*/
struct KeTaskControlBlock* KeGetCurrentTaskParent(void);

END_EXPORT_API

/**
 * @brief Perform task switch that was invoked after IRQ, after DPC, on task yield etc.
 * @attention Do not use this function
*/
INTERNAL void KePerformTaskSwitch(void);

/**
 * @brief Start scheduler
 * @param *continuationTask Continuation function to be executed in a new process - might be NULL
 * @param *continuationContext Context to be passed to the continuation function
 * @attention This function does not return
 * @details Initialize scheduler, create idle and continuation process.
 * Then enable interrupts and start scheduling.
*/
INTERNAL void KeStartScheduler(void (*continuationTask)(void*), void *continuationContext);

/**
 * @brief Join scheduler with a new CPU
 */
INTERNAL void KeJoinScheduler(void);

/**
 * @brief Attach last task to appropriate queue
 * @param cpu CPU number
 * @attention This function is for context switch code use only
 */
INTERNAL void KeAttachLastTask(uint16_t cpu);

#endif