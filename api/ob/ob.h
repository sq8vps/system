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
 * @brief Kernel object types
 */
enum ObObjectType
{
    OB_SPINLOCK = 0x1,
    OB_MUTEX = 0x2,
    OB_SEMAPHORE = 0x3,
    OB_RW_LOCK = 0x4,
    OB_PCB = 0x5,
    OB_TCB = 0x6,
};

/**
 * @brief Header structure for all kernel objects
*/
struct ObObjectHeader
{
    uint32_t magic;
    KeSpinlock lock;
};

#define OBJECT struct ObObjectHeader _object

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


#ifdef __cplusplus
}
#endif

#endif