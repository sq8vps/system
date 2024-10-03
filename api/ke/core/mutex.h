//This header file is generated automatically
#ifndef EXPORTED___API__KE_CORE_MUTEX_H_
#define EXPORTED___API__KE_CORE_MUTEX_H_

#ifdef __cplusplus
extern "C" 
{
#endif

#include <stdint.h>
#include "defines.h"
#include "hal/interrupt.h"

struct KeTaskControlBlock;


/**
 * @brief Acquire mutex immediately if possible. Do not wait otherwise.
*/
#define KE_MUTEX_NO_WAIT 0


/**
 * @brief Acquire mutex when possible. Wait otherwise (indefinitely).
*/
#define KE_MUTEX_NO_TIMEOUT UINT64_MAX


/**
 * @brief A spinlock structure
 * @attention Initialize with KeSpinlockInitializer
*/
typedef struct KeSpinlock
{
    volatile uint32_t lock;
} KeSpinlock;


/**
 * @brief Spinlock initializer. Use it when creating spinlocks.
*/
#define KeSpinlockInitializer {.lock = 0}


/**
 * @brief A mutex (yielding) structure
 * @attention Initialize with KeMutexInitializer
*/
typedef struct KeMutex
{
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
typedef struct KeSemaphore
{
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
 * @brief A read-write lock structure
 * @attention Initialize with KeRwLockInitializer
 */
typedef struct KeRwLock
{
    uint32_t readers;
    uint32_t writers;
    struct KeTaskControlBlock *queueTop;
    struct KeTaskControlBlock *queueBottom;
    KeSpinlock lock;
} KeRwLock;


/**
 * @brief Read-write lock initializer. Use it when creating RW locks.
*/
#define KeRwLockInitializer {.readers = 0, .writers = 0, .queueTop = NULL, .queueBottom = NULL, .lock = KeSpinlockInitializer}


/**
 * @brief Acquire spinlock
 * @param *spinlock Spinlock structure
*/
PRIO KeAcquireSpinlock(KeSpinlock *spinlock);

/**
 * @brief Release spinlock
 * @param *spinlock Spinlock structure
 * @param previousPriority Priority level returned by \a KeAcquireSpinlock()
*/
void KeReleaseSpinlock(KeSpinlock *spinlock, PRIO previousPriority);

/**
 * @brief Acquire mutex (yielding)
 * @param *mutex Mutex structure
*/
void KeAcquireMutex(KeMutex *mutex);


/**
 * @brief Acquire mutex (yielding), but with given timeout
 * @param *mutex Mutex structure
 * @param timeout Timeout in ns or KE_MUTEX_NO_WAIT or KE_MUTEX_NO_TIMEOUT
 * @return True on successful acquistion, false on timeout
*/
bool KeAcquireMutexWithTimeout(KeMutex *mutex, uint64_t timeout);


/**
 * @brief Release mutex
 * @param *mutex Mutex structure
*/
void KeReleaseMutex(KeMutex *mutex);


/**
 * @brief Acquire semaphore (yielding)
 * @param *sem Semaphore structure
 * @note Semaphore max value must be set
*/
void KeAcquireSemaphore(KeSemaphore *sem);


/**
 * @brief Acquire semaphore (yielding), but with given timeout
 * @param *sem Semaphore structure
 * @param timeout Timeout in ns or KE_MUTEX_NO_WAIT or KE_MUTEX_NO_TIMEOUT
 * @return True on successful acquistion, false on timeout
*/
bool KeAcquireSemaphoreWithTimeout(KeSemaphore *sem, uint64_t timeout);


/**
 * @brief Release semaphore
 * @param *sem Semaphore structure
*/
void KeReleaseSemaphore(KeSemaphore *sem);


/**
 * @brief Acquire read-write (yielding), but with given timeout
 * @param *rwLock RW lock structure
 * @param timeout Timeout in ns or KE_MUTEX_NO_WAIT or KE_MUTEX_NO_TIMEOUT
 * @return True on successful acquistion, false on timeout
*/
bool KeAcquireRwLockWithTimeout(KeRwLock *rwLock, bool write, uint64_t timeout);


/**
 * @brief Acquire read-write lock (yielding)
 * @param *rwLock RW lock structure
 * @param write True if writing, false if reading
*/
void KeAcquireRwLock(KeRwLock *rwLock, bool write);


/**
 * @brief Release read-write lock
 * @param *rwLock RW lock structure
*/
void KeReleaseRwLock(KeRwLock *rwLock);


/**
 * @brief Allocate and initialize mutex
 * @return Create mutex or NULL on failure
 */
KeMutex *KeCreateMutex(void);


/**
 * @brief Allocate and initialize spinlock
 * @return Create spinlock or NULL on failure
 */
KeSpinlock *KeCreateSpinlock(void);


/**
 * @brief Allocate and initialize semaphore
 * @return Create semaphore or NULL on failure
 */
KeSemaphore *KeCreateSemaphore(void);


/**
 * @brief Allocate and initialize RW lock
 * @return Create RW lock or NULL on failure
 */
KeRwLock *KeCreateRwLock(void);


/**
 * @brief Destroy mutex
 * @param *mutex Mutex to be destroyed
 */
void KeDestroyMutex(KeMutex *mutex);


/**
 * @brief Destroy spinlock
 * @param *spinlock Spinlock to be destroyed
 */
void KeDestroySpinlock(KeSpinlock *spinlock);


/**
 * @brief Destroy semaphore
 * @param *semaphore Semaphore to be destroyed
 */
void KeDestroySempahore(KeSemaphore *semaphore);


/**
 * @brief Destroy RW lock
 * @param *rwLock RW lock to be destroyed
 */
void KeDestroyRwLock(KeRwLock *rwLock);


#ifdef __cplusplus
}
#endif

#endif