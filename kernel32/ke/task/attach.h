#ifndef KERNEL_ATTACH_H_
#define KERNEL_ATTACH_H_

#include "defines.h"
#include "ke/core/mutex.h"

EXPORT_API

/**
 * @brief Wait for task attachment indefinitely
 */
#define KE_ATTACH_NO_TIMEOUT KE_MUTEX_NO_TIMEOUT

/**
 * @brief Do not wait for task attachment if not available immediately
 */
#define KE_ATTACH_NO_WAIT KE_MUTEX_NO_WAIT

/**
 * @brief Attach kernel thread to another thread memory space
 * @param *target Target task
 * @param timeout Timeout in nanoseconds, \a KE_ATTACH_NO_TIMEOUT or \a KE_ATTACH_NO_WAIT
 * @return Status code
 * @warning Source (current) task must be a kernel mode task (residing in kernel space)
 * @warning Priority level must be = \a HAL_PRIORITY_LEVEL_PASSIVE
 */
STATUS KeAttachToTask(struct KeTaskControlBlock *target, uint64_t timeout);

END_EXPORT_API

#endif