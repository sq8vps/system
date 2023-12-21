#ifndef KERNEL_DPC_H_
#define KERNEL_DPC_H_

#include "defines.h"
#include <stdint.h>

EXPORT
/**
 * @brief A callback function type for DPC worker
*/
typedef void (*KeDpcCallback)(void *context);

EXPORT
/**
 * @brief DPC priority levels
*/
enum KeDpcPriority
{
    KE_DPC_PRIORITY_NORMAL = 0,
    KE_DPC_PRIORITY_LOW,
    KE_DPC_PRIORITY_HIGH,
};

EXPORT
/**
 * @brief Register a Deferred Procedure Call
 * @param priority DPC priority
 * @param callback DPC function pointer
 * @param *context Context to be passed to the worker function
 * @return Status code
 * @attention Processor priority must be > HAL_TASK_PRIORITY_PASSIVE, otherwise the kernel panic occurs.
*/
EXTERN STATUS KeRegisterDpc(enum KeDpcPriority priority, KeDpcCallback callback, void *context);

/**
 * @brief Process all Deferred Procedure Calls if priority level is low enough
*/
INTERNAL void KeProcessDpcQueue(void);

#endif