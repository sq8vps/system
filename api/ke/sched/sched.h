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
 * @brief Put current task to indefinite sleep and yield
 * @note Task can be woken up using \a KeWakeUpTask()
 * @note If there is a wake-up request pending, this function 
 * returns immediately and the task is not put to sleep
 */
void KeWaitForWakeUp(void);

/**
 * @brief Notify and wake up task
 * 
 * Notify sleeping task that tit should wake up.
 * If the target task is blocked because of sleep, it will be unblocked.
 * If the target task is block because of any other event, it will not be unblocked.
 * @param *tcb Target Task Control Block
 */
void KeWakeUpTask(struct KeTaskControlBlock *tcb);


/**
 * @brief Get current task pointer
 * @return Current Task Control Block
*/
struct KeTaskControlBlock* KeGetCurrentTask(void);

/**
 * @brief Get current task's parent process pointer
 * @return Current task's parent ProcessControlBlock
*/
struct KeProcessControlBlock* KeGetCurrentTaskParent(void);


#ifdef __cplusplus
}
#endif

#endif