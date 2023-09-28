#ifndef KERNEL_SCHED_H_
#define KERNEL_SCHED_H_

#include <stdint.h>
#include "defines.h"
#include "ke/task/task.h"

/**
 * @brief Temporarily disable scheduling - increment task switch postponement counter
*/
void KeSchedulerIncrementPostponeCounter(void);

/**
 * @brief Allow scheduler to be enabled again - decrement task switch postponement counter
 * @warning The scheduler waits for the counter to become zero, so it might not be enabled immidiately
*/
void KeSchedulerDecrementPostponeCounter(void);

EXPORT
/**
 * @brief Change task major priority/scheduling policy
 * @param *tcb Task Control Block pointer
 * @param priority New priority level
 * @return Status code
*/
EXTERN STATUS KeChangeTaskMajorPriority(struct KeTaskControlBlock *tcb, enum KeTaskMajorPriority priority);

EXPORT
/**
 * @brief Change task minor priority/scheduling policy
 * @param *tcb Task Control Block pointer
 * @param priority New priority level
 * @return Status code
*/
EXTERN STATUS KeChangeTaskMinorPriority(struct KeTaskControlBlock *tcb, uint8_t priority);

EXPORT
/**
 * @brief Enable task for scheduling
 * @param *tcb Task Control Block
 * @return Status code
*/
EXTERN STATUS KeEnableTask(struct KeTaskControlBlock *tcb);

/**
 * @brief Start scheduler
 * @attention This function return immediately to caller
 * @details Initialize scheduler, create idle process and create task from currently executed code.
 * Then enable interrupts and start scheduling.
*/
void KeSchedulerStart(void);

EXPORT
/**
 * @brief Suspend task and yield to scheduler
 * @attention Program execution continues from this point
*/
EXTERN void KeTaskYield(void);

/**
 * @brief Block task (remove from ready-to-run queue)
 * @param *tcb Task Control Block
 * @param reson Reason for task block
 */
void KeBlockTask(struct KeTaskControlBlock *tcb, enum KeTaskBlockReason reason);

/**
 * @brief Unblock task (insert to ready-to-run queue)
 * @param *tcb Task Control Block
*/
void KeUnblockTask(struct KeTaskControlBlock *tcb);

EXPORT
/**
 * @brief Get current task pointer
 * @return Current Task Control Block
*/
EXTERN struct KeTaskControlBlock* KeGetCurrentTask(void);

#endif