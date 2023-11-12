#ifndef KERNEL_SSE_H_
#define KERNEL_SSE_H_

#include "defines.h"
#include "ke/task/task.h"

/**
 * @brief Initialize SSE
 * @return Status code
*/
INTERNAL STATUS SseInitialize(void);

/**
 * @brief Store SSE, MMX and x87 state for given task
 * @param *tcb Task Control Block
 * @return Status code
*/
INTERNAL STATUS SseStore(struct KeTaskControlBlock *tcb);

/**
 * @brief Restore SSE, MMX and x87 state for given task
 * @param *tcb Task Control Block
 * @return Status code
*/
INTERNAL STATUS SseRestore(struct KeTaskControlBlock *tcb);

#endif