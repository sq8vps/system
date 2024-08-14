//This header file is generated automatically
#ifndef EXPORTED___API__KE_CORE_DPC_H_
#define EXPORTED___API__KE_CORE_DPC_H_

#ifdef __cplusplus
extern "C" 
{
#endif

#include "defines.h"
#include <stdint.h>

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
    KE_DPC_PRIORITY_LOW,
    KE_DPC_PRIORITY_HIGH,

    _KE_DPC_PRIORITY_LIMIT = KE_DPC_PRIORITY_HIGH,
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


#ifdef __cplusplus
}
#endif

#endif