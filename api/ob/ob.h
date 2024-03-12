//This header file is generated automatically
#ifndef EXPORTED___API__OB_OB_H_
#define EXPORTED___API__OB_OB_H_

#ifdef __cplusplus
extern "C" 
{
#endif

#include <stdint.h>
#include "ke/core/mutex.h"
#include "defines.h"
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
 * 
 * This function locks the object, that is, acquires the associated spinlock.
 * Object should be unlocked as soon as possible.
 * @param *object Object to lock
 * @warning This function causes kernel panic when object is not lockable
*/
extern void ObLockObject(void *object);

/**
 * @brief Unlock object
 * 
 * This function unlocks the object, that is, releases the associated spinlock.
 * @param *object Object to lock
 * @warning This function causes kernel panic when object is not unlockable
*/
extern void ObUnlockObject(void *object);


#ifdef __cplusplus
}
#endif

#endif