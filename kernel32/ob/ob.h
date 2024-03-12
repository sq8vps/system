#ifndef KERNEL_OB_H_
#define KERNEL_OB_H_

#include <stdint.h>
#include "ke/core/mutex.h"
#include "defines.h"

EXPORT
/**
 * @brief Header structure for all kernel objects
*/
struct ObObjectHeader
{
    uint32_t magic;
    KeSpinlock lock;
};

EXPORT
/**
 * @brief Lock object
 * 
 * This function locks the object, that is, acquires the associated spinlock.
 * Object should be unlocked as soon as possible.
 * @param *object Object to lock
 * @warning This function causes kernel panic when object is not lockable
*/
EXTERN void ObLockObject(void *object);

EXPORT
/**
 * @brief Unlock object
 * 
 * This function unlocks the object, that is, releases the associated spinlock.
 * @param *object Object to lock
 * @warning This function causes kernel panic when object is not unlockable
*/
EXTERN void ObUnlockObject(void *object);

/**
 * @brief Initialize object header
 * @param *object Object to initialize the header of
 * @attention The object must contain \a ObObjectHeader structure at the beginning
*/
INTERNAL void ObInitializeObjectHeader(void *object);

#endif