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
    volatile uint32_t lock; /**< Lock state */
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
    KeSpinlock lock; /**< Mutex structure lock */
    struct KeTaskControlBlock *head; /**< Queue head */
    struct KeTaskControlBlock *tail; /**< Queue tail */
    struct KeTaskControlBlock *owner; /**< Current mutex owner */
    uint32_t current; /**< Mutex lock state */
} KeMutex;


/**
 * @brief Mutex initializer. Use it when creating mutices.
*/
#define KeMutexInitializer {.current = 0, .head = NULL, .tail = NULL, .owner = NULL, .lock = KeSpinlockInitializer}


/**
 * @brief A semaphore structure
 * @attention Initialize with KeSemaphoreInitializer
*/
typedef struct KeSemaphore
{
    KeSpinlock lock; /**< Semaphore structure lock */
    struct KeTaskControlBlock *head; /**< Queue head */
    struct KeTaskControlBlock *tail; /**< Queue tail */
    uint32_t current; /**< Current count */
    uint32_t max; /**< Max count */
    uint32_t needed; /**< Units requested by the next task in the queue */
} KeSemaphore;


/**
 * @brief Semaphore initializer. Use it when creating semaphores.
*/
#define KeSemaphoreInitializer {.current = 0, .max = 1, .head = NULL, .tail = NULL, .lock = KeSpinlockInitializer}


/**
 * @brief A read-write lock structure
 * @attention Initialize with KeRwLockInitializer
 */
typedef struct KeRwLock
{
    KeSpinlock lock; /**< RW structure lock */
    struct KeTaskControlBlock *head; /**< Queue head */
    struct KeTaskControlBlock *tail; /**< Queue tail */
    uint32_t readers; /**< Current number of readers */
    uint32_t writers; /**< Current number of writers */
    bool write; /**< Mode requested by the next task in the queue */
} KeRwLock;


/**
 * @brief Read-write lock initializer. Use it when creating RW locks.
*/
#define KeRwLockInitializer {.readers = 0, .writers = 0, .head = NULL, .tail = NULL, .lock = KeSpinlockInitializer}


/**
 * @brief Acquire spinlock with high priority level
 * @param *spinlock Spinlock structure
 * @return Previous priority
*/
PRIO KeAcquireSpinlock(KeSpinlock *spinlock);

/**
 * @brief Acquire spinlock with DPC priority level
 * @param *spinlock Spinlock structure
 * @return Previous priority
 */
PRIO KeAcquireDpcLevelSpinlock(KeSpinlock *spinlock);

/**
 * @brief Release spinlock
 * @param *spinlock Spinlock structure
 * @param previousPriority Priority level returned by \a KeAcquireSpinlock()
*/
void KeReleaseSpinlock(KeSpinlock *spinlock, PRIO previousPriority);


/**
 * @brief Acquire mutex (yielding) with given timeout
 * @param *mutex Mutex structure
 * @param timeout Timeout in ns or KE_MUTEX_NO_WAIT or KE_MUTEX_NO_TIMEOUT
 * @return True on successful acquistion, false on timeout
*/
bool KeAcquireMutexEx(KeMutex *mutex, uint64_t timeout);

/**
 * @brief Acquire mutex (yielding)
 * @param mutex Mutex structure
 * @return Always true
*/
#define KeAcquireMutex(mutex) KeAcquireMutexEx(mutex, KE_MUTEX_NO_TIMEOUT)

/**
 * @brief Release mutex
 * @param *mutex Mutex structure
*/
void KeReleaseMutex(KeMutex *mutex);

/**
 * @brief Acquire semaphore (yielding) with given timeout
 * @param *sem Semaphore structure
 * @param units Number of units to acquire
 * @param timeout Timeout in ns or KE_MUTEX_NO_WAIT or KE_MUTEX_NO_TIMEOUT
 * @return True on successful acquistion, false on timeout
 * @note Returns false immediately if number of units exceeds semaphore's maximum
*/
bool KeAcquireSemaphoreEx(KeSemaphore *sem, uint32_t units, uint64_t timeout);


/**
 * @brief Acquire semaphore (yielding)
 * @param sem Semaphore structure
 * @param units Number of units to acquire
 * @return True if acquistion successful, false if number of units exceeds semaphore's maximum
*/
#define KeAcquireSemaphore(sem, units) KeAcquireSemaphoreEx(sem, units, KE_MUTEX_NO_TIMEOUT)


/**
 * @brief Release semaphore
 * @param *sem Semaphore structure
 * @param units Number of units to release
*/
void KeReleaseSemaphore(KeSemaphore *sem, uint32_t units);


/**
 * @brief Acquire read-write (yielding) with given timeout
 * @param *rwLock RW lock structure
 * @param timeout Timeout in ns or KE_MUTEX_NO_WAIT or KE_MUTEX_NO_TIMEOUT
 * @return True on successful acquistion, false on timeout
*/
bool KeAcquireRwLockEx(KeRwLock *rwLock, bool write, uint64_t timeout);

/**
 * @brief Acquire read-write lock (yielding)
 * @param rwLock RW lock structure
 * @param write True if writing, false if reading
 * @return Always true
*/
#define KeAcquireRwLock(rwLock, write) KeAcquireRwLockEx(rwLock, write, KE_MUTEX_NO_TIMEOUT)

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
 * @param initial Initial semaphore count
 * @param max Max semaphore count
 * @return Create semaphore or NULL on failure
 */
KeSemaphore *KeCreateSemaphore(uint32_t initial, uint32_t max);


/**
 * @brief Allocate and initialize RW lock
 * @return Created RW lock or NULL on failure
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