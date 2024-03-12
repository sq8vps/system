#ifndef KERNEL_SCHED_H_
#define KERNEL_SCHED_H_

#include <stdint.h>
#include "defines.h"
#include "ke/task/task.h"

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
 * @brief Perform task switch immediately if new task is available
 * @attention Do not use this function. KeSchedule() should be used first to prepare new task.
*/
INTERNAL extern void KePerformTaskSwitch(void);

/**
 * @brief Perform task switch if previous task was preempted and new task is available
 * @attention Do not use this function. KeSchedule() should be used first to prepare new task.
 * @attention This function works only for tasks scheduled from task switch DPC.
*/
INTERNAL void KePerformPreemptedTaskSwitch(void);

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

EXPORT
/**
 * @brief Get current task pointer
 * @return Current Task Control Block
*/
EXTERN struct KeTaskControlBlock* KeGetCurrentTask(void);

#endif