#ifndef KERNEL_SCHED_RR_H_
#define KERNEL_SCHED_RR_H_

#include "defines.h"

struct KeTaskControlBlock;

/**
 * @brief Attach task to the round-robin scheduler queue
 * 
 * This function is used to attach a task that either returned from "blocked" state to "ready" state, or was preempted, or yielded to the scheduler
 * back to the round-robin queue.
 * @param *tcb TCB of the task to be attached
 * @attention This function does \a not lock the TCB
 */
INTERNAL void KeRrQueueTask(struct KeTaskControlBlock *tcb);

/**
 * @brief Get next task from the round-robin scheduler
 * @param cpu CPU number
 * @param *current Currently running task
 * @param *slice Output time slice in nanoseconds
 * @return TCB of the task to be run or \a nullptr if no RR-schedulable task is available
 */
INTERNAL struct KeTaskControlBlock* KeRrGetNextTask(uint32_t cpu, struct KeTaskControlBlock *current, uint64_t *slice);

#endif