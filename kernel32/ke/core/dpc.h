#ifndef KERNEL_DPC_H_
#define KERNEL_DPC_H_

#include "defines.h"
#include <stdint.h>

EXPORT
typedef void (*KeDpcCallback)(void *context);

EXPORT
enum KeDpcPriority
{
    KE_DPC_PRIORITY_NORMAL = 0,
    KE_DPC_PRIORITY_LOW,
    KE_DPC_PRIORITY_HIGH,
};

EXPORT
/**
 * @brief Register a Deferred Procedure Call from ISR
 * @param priority DPC priority
 * @param irq IRQ value
 * @param callback DPC function pointer
 * @param *context Context to be passed to the worker function
 * @return Status code 
*/
EXTERN STATUS KeRegisterDpcFromIsr(enum KeDpcPriority priority, uint8_t irq, KeDpcCallback callback, void *context);

EXPORT
/**
 * @brief Register a Deferred Procedure Call
 * @param priority DPC priority
 * @param callback DPC function pointer
 * @param *context Context to be passed to the worker function
 * @return Status code 
*/
EXTERN STATUS KeRegisterDpc(enum KeDpcPriority priority, KeDpcCallback callback, void *context);

/**
 * @brief Process all Deferred Procedure Calls
*/
INTERNAL void KeProcessDpcQueue(void);

#endif