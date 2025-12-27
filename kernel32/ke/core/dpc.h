/**
 * @file dpc.h
 * @brief Deferred Procedure Call support
 * @ingroup ke_core
 */

#ifndef KERNEL_DPC_H_
#define KERNEL_DPC_H_

#include "defines.h"
#include <stdint.h>

/**
 * @addtogroup ke Core kernel module
 * 
 * This module is responsible for hardware-independent kernel functionalities, such as task handling, scheduling,
 * mutual exclusion, or system calls.
 */

/**
 * @addtogroup ke_core Fundamental kernel API
 * @ingroup ke
 * @{
 */

EXPORT_API

/**
 * @brief A callback function type for DPC worker
*/
typedef void (*KeDpcCallback)(void *context);


/**
 * @brief DPC priority levels
*/
enum KeDpcPriority
{
    KE_DPC_PRIORITY_NORMAL = 0,
    KE_DPC_PRIORITY_LOW = -1,
    KE_DPC_PRIORITY_HIGH = 1,

    KE_DPC_PRIORITY_COUNT = 3,
};


/**
 * @brief Register a Deferred Procedure Call
 * @param priority DPC priority
 * @param callback DPC function pointer
 * @param *context Context to be passed to the worker function
 * @return Status code
 * @attention Processor priority must be > HAL_PRIORITY_LEVEL_PASSIVE, otherwise the kernel panic occurs.
*/
STATUS KeRegisterDpc(enum KeDpcPriority priority, KeDpcCallback callback, void *context);

END_EXPORT_API

/**
 * @brief Process all Deferred Procedure Calls if priority level is low enough
 * @kinternal
*/
INTERNAL void KeProcessDpcQueue(void);

/**
 * @brief Initialize Deffered Procedure Call module
 * @kinternal
 * @return Status code
*/
INTERNAL STATUS KeDpcInitialize(void);

/**
 * @}
 */

#endif