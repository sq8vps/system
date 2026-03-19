#ifndef KERNEL_SCHED_FCFS_H_
#define KERNEL_SCHED_FCFS_H_

#include "defines.h"

struct KeTaskControlBlock;

/**
 * @brief Attach task to the FCFS scheduler queue
 * 
 * This function is used to attach a task that either returned from "blocked" state to "ready" state, or yielded to the scheduler
 * back to the round-robin queue.
 * @param *tcb TCB of the task to be attached
 */
INTERNAL void KeFcfsQueueTask(struct KeTaskControlBlock *tcb);

/**
 * @brief Get next task from the FCFS scheduler
 * @param cpu CPU number
 * @param *slice Output time slice in nanoseconds - \b always \b zero
 * @return TCB of the task to be run or \a nullptr if no FCFS-schedulable task is available
 */
INTERNAL struct KeTaskControlBlock* KeFcfsGetNextTask(uint32_t cpu, uint64_t *slice);

#endif