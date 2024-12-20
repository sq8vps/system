#ifndef KERNEL_EX_WORKER_H_
#define KERNEL_EX_WORKER_H_

#include <stdint.h>
#include "defines.h"

EXPORT_API

struct KeTaskControlBlock;

/**
 * @brief Create kernel worker thread
 * @param *entry Thread entry point
 * @param *entryContext Context to be passed to entry point
 * @param **tcb Output Task Control Block
 * @return Status code
 */
STATUS ExCreateKernelWorker(void(*entry)(void *), void *entryContext, struct KeTaskControlBlock **tcb);

END_EXPORT_API

#endif