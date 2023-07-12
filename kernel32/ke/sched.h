#ifndef KERNEL32_SCHED_H_
#define KERNEL32_SCHED_H_

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

EXPORT STATUS KeChangeTaskMajorPriority(struct TaskControlBlock *tcb, enum TaskMajorPriority priority);

EXPORT STATUS KeChangeTaskMinorPriority(struct TaskControlBlock *tcb, uint8_t priority);

STATUS KeChangeTaskState(struct TaskControlBlock *tcb, enum TaskState state);

void KeSchedulerStart(void);

#endif