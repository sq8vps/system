#ifndef KERNEL_SCHED_CFS_H_
#define KERNEL_SCHED_CFS_H_

#include "defines.h"

struct KeTaskControlBlock;

/**
 * @brief Attach task to the CFS scheduler queue
 * 
 * This function is used to attach a task that either returned from "blocked" state to "ready" state, or was preempted, or yielded to the scheduler
 * back to the round-robin queue.
 * @param *tcb TCB of the task to be attached
 * @attention This function does \a not lock the TCB
 */
INTERNAL void KeCfsQueueTask(struct KeTaskControlBlock *tcb);

/**
 * @brief Get next task from the CFS scheduler
 * @param cpu CPU number
 * @param *current Currently running task
 * @param *slice Output time slice in nanoseconds
 * @return TCB of the task to be run or \a nullptr if no CFS-schedulable task is available
 */
INTERNAL struct KeTaskControlBlock* KeCfsGetNextTask(uint32_t cpu, struct KeTaskControlBlock *current, uint64_t *slice);

#endif