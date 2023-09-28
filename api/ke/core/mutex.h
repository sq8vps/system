//This header file is generated automatically
#ifndef EXPORTED___API__KE_CORE_MUTEX_H_
#define EXPORTED___API__KE_CORE_MUTEX_H_

#ifdef __cplusplus
extern "C" 
{
#endif

#include <stdint.h>
#include "defines.h"
#include "ke/task/task.h"
/**
 * @brief Acquire mutex immediately if possible. Do not wait otherwise.
*/
#define KE_MUTEX_NO_WAIT 0

/**
 * @brief Acquire mutex when possible. Wait otherwise
*/
#define KE_MUTEX_NORMAL 0xFFFFFFFFFFFFFFFF

/**
 * @brief A spinlock structure
 * @attention Initialize with KeSpinlockInitializer
*/
typedef struct _KeSpinlock
{
    uint16_t lock;
    uint32_t eflags;
} KeSpinlock;

/**
 * @brief Spinlock initializer. Use it when creating spinlocks.
*/
#define KeSpinlockInitializer {.lock = 0, .eflags = 0}

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

/**
 * @brief Mutex initializer. Use it when creating mutices.
*/
#define KeMutexInitializer {.lock = 0, .queueTop = NULL, .queueBottom = NULL, .spinlock = KeSpinlockInitializer}

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

/**
 * @brief Semaphore initializer. Use it when creating semaphores.
*/
#define KeSemaphoreInitializer {.current = 0, .max = 1, .queueTop = NULL, .queueBottom = NULL, .spinlock = KeSpinlockInitializer}

/**
 * @brief Acquire spinlock
 * @param *spinlock Spinlock structure
*/
extern void KeAcquireSpinlock(KeSpinlock *spinlock);

/**
 * @brief Release spinlock
 * @param *spinlock Spinlock structure
*/
extern void KeReleaseSpinlock(KeSpinlock *spinlock);

/**
 * @brief Acquire mutex (yielding)
 * @param *mutex Mutex structure
*/
extern void KeAcquireMutex(KeMutex *mutex);

/**
 * @brief Acquire mutex (yielding), but with given timeout
 * @param *mutex Mutex structure
 * @param timeout Timeout in ns or KE_MUTEX_NO_WAIT or KE_MUTEX_NORMAL
 * @return True on successful acquistion, false on timeout
*/
extern bool KeAcquireMutexWithTimeout(KeMutex *mutex, uint64_t timeout);

/**
 * @brief Release mutex
 * @param *mutex Mutex structure
*/
extern void KeReleaseMutex(KeMutex *mutex);

/**
 * @brief Acquire semaphore (yielding)
 * @param *sem Semaphore structure
 * @note Semaphore max value must be set
*/
extern void KeAcquireSemaphore(KeSemaphore *sem);

/**
 * @brief Acquire semaphore (yielding), but with given timeout
 * @param *sem Semaphore structure
 * @param timeout Timeout in ns or KE_MUTEX_NO_WAIT or KE_MUTEX_NORMAL
 * @return True on successful acquistion, false on timeout
*/
extern bool KeAcquireSemaphoreWithTimeout(KeSemaphore *sem, uint64_t timeout);

/**
 * @brief Release semaphore
 * @param *sem Semaphore structure
*/
extern void KeReleaseSemaphore(KeSemaphore *sem);


#ifdef __cplusplus
}
#endif

#endif