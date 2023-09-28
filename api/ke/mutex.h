//This header file is generated automatically
#ifndef API___API__KE_MUTEX_H_
#define API___API__KE_MUTEX_H_

#ifdef __cplusplus
extern "C" 
{
#endif

#include <stdint.h>
#include "defines.h"
#include "task.h"
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
    uint16_t lock;
    struct KeTaskControlBlock *queueTop;
    struct KeTaskControlBlock *queueBottom;
    KeSpinlock spinlock;
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
    uint32_t current;
    uint32_t max;
    struct KeTaskControlBlock *queueTop;
    struct KeTaskControlBlock *queueBottom;
    KeSpinlock spinlock;
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
 * @brief Release semaphore
 * @param *sem Semaphore structure
*/
extern void KeReleaseSemaphore(KeSemaphore *sem);


#ifdef __cplusplus
}
#endif

#endif