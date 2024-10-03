#ifndef KERNEL_OB_H_
#define KERNEL_OB_H_

#include <stdint.h>
#include "ke/core/mutex.h"
#include "defines.h"

EXPORT_API

/**
 * @brief Header structure for all kernel objects
*/
struct ObObjectHeader
{
    uint32_t magic;
    KeSpinlock lock;
};

/**
 * @brief Lock object
 * @param *object Object pointer
 * @return Previous task priority to be passed to \a ObUnlockObject()
 */
PRIO ObLockObject(void *object);

/**
 * @brief Unlock object
 * @param *object Object pointer
 * @param previousPriority Previous task priority obtained from \a ObLockObject()
 */
void ObUnlockObject(void *object, PRIO previousPriority);

END_EXPORT_API

/**
 * @brief Initialize object header
 * @param *object Object to initialize the header of
 * @attention The object must contain \a ObObjectHeader structure at the beginning
*/
INTERNAL void ObInitializeObjectHeader(void *object);

#endif