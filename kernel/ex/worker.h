/**
 * @file worker.h
 * @brief Kernel worker thread manipulation routines
 * @ingroup exec
 */

#ifndef KERNEL_EX_WORKER_H_
#define KERNEL_EX_WORKER_H_

#include <stdint.h>
#include "defines.h"

DRIVER_API

struct KeTaskControlBlock;

/**
 * @addtogroup ex_worker Kernel worker thread manipulation routines
 * @ingroup exec
 * @{
*/


/**
 * @brief Create kernel worker thread
 * @param *entry Thread entry point
 * @param *entryContext Context to be passed to entry point
 * @param **tcb Output Task Control Block
 * @return Status code
 */
STATUS ExCreateKernelWorker(void(*entry)(void *), void *entryContext, struct KeTaskControlBlock **tcb);

/**
 * @}
 */

END_DRIVER_API

#endif