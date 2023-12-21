#ifndef KERNEL_MUTEX_H_
#define KERNEL_MUTEX_H_

#include <stdint.h>
#include "defines.h"
#include "ke/task/task.h"

EXPORT
/**
 * @brief Acquire mutex immediately if possible. Do not wait otherwise.
*/
#define KE_MUTEX_NO_WAIT 0

EXPORT
/**
 * @brief Acquire mutex when possible. Wait otherwise
*/
#define KE_MUTEX_NORMAL 0xFFFFFFFFFFFFFFFF

EXPORT
/**
 * @brief A spinlock structure
 * @attention Initialize with KeSpinlockInitializer
*/
typedef struct _KeSpinlock
{
    uint16_t lock;
    uint32_t eflags;
} KeSpinlock;

EXPORT
/**
 * @brief Spinlock initializer. Use it when creating spinlocks.
*/
#define KeSpinlockInitializer {.lock = 0, .eflags = 0}

EXPORT
/**
 * @brief A mutex (yielding) structure
 * @attention Initialize with KeMutexInitializer
*/
typedef struct _KeMutex
{
    //WARNING! Keep queueTop, queueBottom and spinlock at the beginning of each semaphore-like structure
    struct KeTaskControlBlock *queueTop;
    struct KeTaskControlBlock *queueBottom;
    KeSpinlock spinlock;
    uint16_t lock;
} KeMutex;

EXPORT
/**
 * @brief Mutex initializer. Use it when creating mutices.
*/
#define KeMutexInitializer {.lock = 0, .queueTop = NULL, .queueBottom = NULL, .spinlock = KeSpinlockInitializer}

EXPORT
/**
 * @brief A semaphore structure
 * @attention Initialize with KeSemaphoreInitializer
*/
typedef struct _KeSemaphore
{
    //WARNING! Keep queueTop, queueBottom and spinlock at the beginning of each semaphore-like structure
    struct KeTaskControlBlock *queueTop;
    struct KeTaskControlBlock *queueBottom;
    KeSpinlock spinlock;
    uint32_t current;
    uint32_t max;
} KeSemaphore;

EXPORT
/**
 * @brief Semaphore initializer. Use it when creating semaphores.
*/
#define KeSemaphoreInitializer {.current = 0, .max = 1, .queueTop = NULL, .queueBottom = NULL, .spinlock = KeSpinlockInitializer}

EXPORT
/**
 * @brief Acquire spinlock
 * @param *spinlock Spinlock structure
*/
EXTERN void KeAcquireSpinlock(KeSpinlock *spinlock);

EXPORT
/**
 * @brief Release spinlock
 * @param *spinlock Spinlock structure
*/
EXTERN void KeReleaseSpinlock(KeSpinlock *spinlock);

EXPORT
/**
 * @brief Acquire mutex (yielding)
 * @param *mutex Mutex structure
*/
EXTERN void KeAcquireMutex(KeMutex *mutex);

EXPORT
/**
 * @brief Acquire mutex (yielding), but with given timeout
 * @param *mutex Mutex structure
 * @param timeout Timeout in ns or KE_MUTEX_NO_WAIT or KE_MUTEX_NORMAL
 * @return True on successful acquistion, false on timeout
*/
EXTERN bool KeAcquireMutexWithTimeout(KeMutex *mutex, uint64_t timeout);

EXPORT
/**
 * @brief Release mutex
 * @param *mutex Mutex structure
*/
EXTERN void KeReleaseMutex(KeMutex *mutex);

EXPORT
/**
 * @brief Acquire semaphore (yielding)
 * @param *sem Semaphore structure
 * @note Semaphore max value must be set
*/
EXTERN void KeAcquireSemaphore(KeSemaphore *sem);

EXPORT
/**
 * @brief Acquire semaphore (yielding), but with given timeout
 * @param *sem Semaphore structure
 * @param timeout Timeout in ns or KE_MUTEX_NO_WAIT or KE_MUTEX_NORMAL
 * @return True on successful acquistion, false on timeout
*/
EXTERN bool KeAcquireSemaphoreWithTimeout(KeSemaphore *sem, uint64_t timeout);

EXPORT
/**
 * @brief Release semaphore
 * @param *sem Semaphore structure
*/
EXTERN void KeReleaseSemaphore(KeSemaphore *sem);

/**
 * @brief Check and unblock tasks waiting for timed mutex or spinlock
*/
INTERNAL void KeTimedExclusionRefresh(void);

#endif