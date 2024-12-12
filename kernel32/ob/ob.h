#ifndef KERNEL_OB_H_
#define KERNEL_OB_H_

#include <stdint.h>
#include "ke/core/mutex.h"
#include "defines.h"

EXPORT_API

/**
 * @brief Kernel object types
 */
enum ObObjectType
{
    OB_SPINLOCK = 0x1, /**< Spinlock */
    OB_MUTEX = 0x2, /**< Mutex */
    OB_SEMAPHORE = 0x3, /**< Semaphore */
    OB_RW_LOCK = 0x4, /**< Read/write lock */
    OB_PCB = 0x5, /**< Process control block */
    OB_TCB = 0x6, /**< Task/thread control block */
    OB_FILE = 0x7, /**< File descriptor */
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

END_EXPORT_API

/**
 * @brief Initialize object header
 * @param *object Object to initialize the header of
 * @attention The object must contain \a ObObjectHeader structure at the beginning
*/
INTERNAL void ObInitializeObjectHeader(void *object);

#endif