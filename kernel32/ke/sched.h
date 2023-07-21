#ifndef KERNEL_SCHED_H_
#define KERNEL_SCHED_H_

#include <stdint.h>
#include "defines.h"
#include "task.h"

/**
 * @brief Temporarily disable scheduling - increment task switch postponement counter
*/
void KeSchedulerIncrementPostponeCounter(void);

/**
 * @brief Allow scheduler to be enabled again - decrement task switch postponement counter
 * @warning The scheduler waits for the counter to become zero, so it might not be enabled immidiately
*/
void KeSchedulerDecrementPostponeCounter(void);

/**
 * @brief Change task major priority/scheduling policy
 * @param *tcb Task Control Block pointer
 * @param priority New priority level
 * @return Status code
*/
EXPORT STATUS KeChangeTaskMajorPriority(struct TaskControlBlock *tcb, enum TaskMajorPriority priority);

/**
 * @brief Change task minor priority/scheduling policy
 * @param *tcb Task Control Block pointer
 * @param priority New priority level
 * @return Status code
*/
EXPORT STATUS KeChangeTaskMinorPriority(struct TaskControlBlock *tcb, uint8_t priority);

/**
 * @brief Enable task for scheduling
 * @param *tcb Task Control Block
 * @return Status code
*/
EXPORT STATUS KeEnableTask(struct TaskControlBlock *tcb);

/**
 * @brief Start scheduler
 * @attention This function return immediately to caller
 * @details Initialize scheduler, create idle process and create task from currently executed code.
 * Then enable interrupts and start scheduling.
*/
void KeSchedulerStart(void);

/**
 * @brief Suspend task and yield to scheduler
 * @attention Program execution continues from this point
*/
EXPORT void KeTaskYield(void);

#endif