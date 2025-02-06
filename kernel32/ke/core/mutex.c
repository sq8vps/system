#include "mutex.h"
#include "ke/sched/sched.h"
#include "panic.h"
#include "hal/time.h"
#include "it/it.h"
#include "hal/arch.h"
#include "mm/heap.h"
#include "assert.h"

/**
 * @brief Structure for timed locks queue
 */
static struct
{
    struct KeTaskControlBlock *head; /**< Queue head, sorted earliest deadline first */
    KeSpinlock lock; /**< Queue lock */
    volatile uint64_t earliest; /**< Earliest deadline = deadline of the first task */
} KeLockingQueue = {.head = NULL, .lock = KeSpinlockInitializer, .earliest = 0};


static void KeInsertToLockList(struct KeTaskControlBlock *tcb, uint64_t timeout)
{
    timeout += HalGetTimestamp();
    PRIO prio = KeAcquireSpinlock(&(KeLockingQueue.lock));
    ASSERT(!tcb->scheduling.block.timeout.next && !tcb->scheduling.block.timeout.previous);
    tcb->scheduling.block.timeout.next  = NULL;
    tcb->scheduling.block.timeout.previous  = NULL;
    tcb->scheduling.block.timeout.until = timeout;
    if(NULL == KeLockingQueue.head)
    {
        KeLockingQueue.head = tcb;
        __atomic_store_n(&(KeLockingQueue.earliest), timeout, __ATOMIC_RELAXED);
    }
    else
    {
        struct KeTaskControlBlock *s = KeLockingQueue.head;
        //keep list sorted: the earliest deadline is first
        while(NULL != s->scheduling.block.timeout.next)
        {
            if((s->scheduling.block.timeout.until <= timeout) 
                && (s->scheduling.block.timeout.next->scheduling.block.timeout.until > timeout))
            {
                tcb->scheduling.block.timeout.next = s->scheduling.block.timeout.next;
                if(NULL != tcb->scheduling.block.timeout.next)
                    tcb->scheduling.block.timeout.next->scheduling.block.timeout.previous = tcb;
                break;
            }
            s = s->scheduling.block.timeout.next;
        }
        ASSERT(s != tcb);
        s->scheduling.block.timeout.next = tcb;
        tcb->scheduling.block.timeout.previous = s;
    }
    KeReleaseSpinlock(&(KeLockingQueue.lock), prio);
}

static inline void KeRemoveFromLockList(struct KeTaskControlBlock *tcb)
{
    if(KeLockingQueue.head == tcb)
    {
        KeLockingQueue.head = tcb->scheduling.block.timeout.next;
        if(NULL != KeLockingQueue.head)
            __atomic_store_n(&(KeLockingQueue.earliest), KeLockingQueue.head->scheduling.block.timeout.until, __ATOMIC_RELAXED);
    }
    if(NULL != tcb->scheduling.block.timeout.previous)
        tcb->scheduling.block.timeout.previous->scheduling.block.timeout.next = tcb->scheduling.block.timeout.next;
    if(NULL != tcb->scheduling.block.timeout.next)
        tcb->scheduling.block.timeout.next->scheduling.block.timeout.previous = tcb->scheduling.block.timeout.previous;
    tcb->scheduling.block.timeout.next = NULL;
    tcb->scheduling.block.timeout.previous = NULL;
    tcb->scheduling.block.timeout.until = 0;
}

PRIO KeAcquireSpinlock(KeSpinlock *spinlock)
{
    PRIO prio = HalRaisePriorityLevel(HAL_PRIORITY_LEVEL_SPINLOCK);
#ifndef SMP
    if(0 != spinlock->lock)
        KePanicEx(BUSY_MUTEX_ACQUIRED, (uintptr_t)spinlock, 0, 0, 0);
    spinlock->lock = 1;
#else
    while(1)
    {
        //obtain previous value and try to set atomically
        //if previous value was 0, then we've acquired the lock
        if(0 == __atomic_exchange_n(&(spinlock->lock), 1, __ATOMIC_SEQ_CST))
            break;

        //else we haven't acquired the lock
        //loop until the lock appears to be free
        //do not use atomic operations to avoid locking CPU memory bus
        //and optimize such tight loop
        //after we detect possibly free lock, then do the atomic exchange again
        while(0 != spinlock->lock)
            TIGHT_LOOP_HINT();
    }
    
#endif
    return prio;
}

PRIO KeAcquireDpcLevelSpinlock(KeSpinlock *spinlock)
{
    PRIO prio = HalRaisePriorityLevel(HAL_PRIORITY_LEVEL_DPC);
#ifndef SMP
    if(0 != spinlock->lock)
        KePanicEx(BUSY_MUTEX_ACQUIRED, (uintptr_t)spinlock, 0, 0, 0);
    spinlock->lock = 1;
#else
    while(1)
    {
        //obtain previous value and try to set atomically
        //if previous value was 0, then we've acquired the lock
        if(0 == __atomic_exchange_n(&(spinlock->lock), 1, __ATOMIC_SEQ_CST))
            break;

        //else we haven't acquired the lock
        //loop until the lock appears to be free
        //do not use atomic operations to avoid locking CPU memory bus
        //and optimize such tight loop
        //after we detect possibly free lock, then do the atomic exchange again
        while(0 != spinlock->lock)
            TIGHT_LOOP_HINT();
    }
    
#endif
    return prio;
}

void KeReleaseSpinlock(KeSpinlock *spinlock, PRIO previousPriority)
{
#ifndef SMP
    if(0 == spinlock->lock)
        KePanicEx(UNACQUIRED_MUTEX_RELEASED, (uintptr_t)spinlock, 0, 0, 0);
    spinlock->lock = 0;
#else
    if(0 == __atomic_load_n(&spinlock->lock, __ATOMIC_SEQ_CST))
        KePanicEx(UNACQUIRED_MUTEX_RELEASED, (uintptr_t)spinlock, 0, 0, 0);
    __atomic_store_n(&spinlock->lock, 0, __ATOMIC_SEQ_CST);
#endif
    HalLowerPriorityLevel(previousPriority);
}

bool KeAcquireMutexEx(KeMutex *mutex, uint64_t timeout)
{
    HalCheckPriorityLevel(HAL_PRIORITY_LEVEL_PASSIVE, HAL_PRIORITY_LEVEL_PASSIVE);
    struct KeTaskControlBlock *tcb = KeGetCurrentTask();
    PRIO prio = KeAcquireSpinlock(&(mutex->lock));
    if((tcb != mutex->owner) && (NULL != mutex->owner))
    {
        if(KE_MUTEX_NO_WAIT == timeout)
        {
            KeReleaseSpinlock(&(mutex->lock), prio);
            return false;
        }
        else
        {
            KeBlockTask(tcb, TASK_BLOCK_MUTEX);
            PRIO tcbPrio = KeAcquireSpinlock(&(tcb->scheduling.lock));
            tcb->scheduling.block.next = NULL;
            if(NULL != mutex->tail)
            {
                PRIO prio = KeAcquireSpinlock(&(mutex->tail->scheduling.lock));
                mutex->tail->scheduling.block.next = tcb;
                KeReleaseSpinlock(&(mutex->tail->scheduling.lock), prio);
                tcb->scheduling.block.previous = mutex->tail;
                mutex->tail = tcb;
            }
            else
            {
                tcb->scheduling.block.previous = NULL;
                mutex->head = tcb;
                mutex->tail = tcb;
            }
            tcb->scheduling.block.mutex = mutex;
            tcb->scheduling.block.acquired = false;
            
            if(timeout < KE_MUTEX_NO_TIMEOUT)
                KeInsertToLockList(tcb, timeout);
            else
                tcb->scheduling.block.timeout.until = 0;
            KeReleaseSpinlock(&(tcb->scheduling.lock), tcbPrio);
            KeReleaseSpinlock(&(mutex->lock), prio);
            KeTaskYield(); //suspend task and wait for an event

            return tcb->scheduling.block.acquired;
        }
    }
    else
    {
        mutex->owner = tcb;
        mutex->current++;
        KeReleaseSpinlock(&(mutex->lock), prio);
        return true;
    }
}

void KeReleaseMutex(KeMutex *mutex)
{
    if(NULL == mutex)
        return;
    PRIO queuePrio = KeAcquireSpinlock(&(KeLockingQueue.lock));
    PRIO prio = KeAcquireSpinlock(&(mutex->lock));
    if(unlikely((NULL == mutex->owner) || (0 == mutex->current)))
        KePanicEx(UNACQUIRED_MUTEX_RELEASED, 1, (uintptr_t)mutex, 0, 0);
    mutex->current--;
    if(0 == mutex->current)
    {
        if(NULL == mutex->head)
        {
            mutex->owner = NULL;
            KeReleaseSpinlock(&(mutex->lock), prio);
            KeReleaseSpinlock(&(KeLockingQueue.lock), queuePrio);
            return;
        }
        struct KeTaskControlBlock *next = mutex->head;
        PRIO prio = KeAcquireSpinlock(&(next->scheduling.lock));
        mutex->head = next->scheduling.block.next;
        if(NULL != mutex->head)
        {
            PRIO prio = KeAcquireSpinlock(&(mutex->head->scheduling.lock));
            mutex->head->scheduling.block.previous = NULL;
            KeReleaseSpinlock(&(mutex->head->scheduling.lock), prio);
        }
        else
            mutex->tail = NULL;
        next->scheduling.block.next = NULL;
        next->scheduling.block.previous = NULL;
        next->scheduling.block.mutex = NULL;
        next->scheduling.block.acquired = true;
        mutex->owner = next;
        mutex->current = 1;
        KeReleaseSpinlock(&(next->scheduling.lock), prio);
        if(0 != next->scheduling.block.timeout.until)
            KeRemoveFromLockList(next);
        KeUnblockTask(next);
    }
    KeReleaseSpinlock(&(mutex->lock), prio);
    KeReleaseSpinlock(&(KeLockingQueue.lock), queuePrio);
}

bool KeAcquireSemaphoreEx(KeSemaphore *sem, uint32_t units, uint64_t timeout)
{
    HalCheckPriorityLevel(HAL_PRIORITY_LEVEL_PASSIVE, HAL_PRIORITY_LEVEL_PASSIVE);
    if(units > sem->max)
        return false;
    struct KeTaskControlBlock *tcb = KeGetCurrentTask();
    PRIO prio = KeAcquireSpinlock(&(sem->lock));
    if((sem->max - sem->current) < units)
    {
        if(KE_MUTEX_NO_WAIT == timeout)
        {
            KeReleaseSpinlock(&(sem->lock), prio);
            return false;
        }
        else
        {
            KeBlockTask(tcb, TASK_BLOCK_MUTEX);
            PRIO tcbPrio = KeAcquireSpinlock(&(tcb->scheduling.lock));
            tcb->scheduling.block.next = NULL;
            if(NULL != sem->tail)
            {
                PRIO prio = KeAcquireSpinlock(&(sem->tail->scheduling.lock));
                sem->tail->scheduling.block.next = tcb;
                KeReleaseSpinlock(&(sem->tail->scheduling.lock), prio);
                tcb->scheduling.block.previous = sem->tail;
                sem->tail = tcb;
            }
            else
            {
                tcb->scheduling.block.previous = NULL;
                sem->head = tcb;
                sem->tail = tcb;
                sem->needed = units;
            }
            tcb->scheduling.block.semaphore = sem;
            tcb->scheduling.block.acquired = false;
            tcb->scheduling.block.count = units;
            
            if(timeout < KE_MUTEX_NO_TIMEOUT)
                KeInsertToLockList(tcb, timeout);
            else
                tcb->scheduling.block.timeout.until = 0;
            KeReleaseSpinlock(&(tcb->scheduling.lock), tcbPrio);
            KeReleaseSpinlock(&(sem->lock), prio);
            KeTaskYield(); //suspend task and wait for an event

            return tcb->scheduling.block.acquired;
        }
    }
    else
    {
        sem->current += units;
        KeReleaseSpinlock(&(sem->lock), prio);
        return true;
    }
}

static inline void KeSemaphoreProcessNextOnRelease(KeSemaphore *sem)
{
    if((sem->max - sem->current) >= sem->needed)
    {
        if(NULL == sem->head)
        {
            sem->needed = 0;
            return;
        }
        struct KeTaskControlBlock *next = sem->head;
        PRIO prio = KeAcquireSpinlock(&(next->scheduling.lock));

        if((sem->max - sem->current) >= next->scheduling.block.count)
        {
            sem->head = next->scheduling.block.next;
            if(NULL != sem->head)
            {
                PRIO prio = KeAcquireSpinlock(&(sem->head->scheduling.lock));
                sem->head->scheduling.block.previous = NULL;
                sem->needed = sem->head->scheduling.block.count;
                KeReleaseSpinlock(&(sem->head->scheduling.lock), prio);
            }
            else
            {
                sem->tail = NULL;
                sem->needed = 0;
            }
            next->scheduling.block.next = NULL;
            next->scheduling.block.previous = NULL;
            next->scheduling.block.semaphore = NULL;
            next->scheduling.block.acquired = true;
            sem->current += next->scheduling.block.count;
            KeReleaseSpinlock(&(next->scheduling.lock), prio);
            if(0 != next->scheduling.block.timeout.until)
                KeRemoveFromLockList(next);
            KeUnblockTask(next);
        }
        else
            KeReleaseSpinlock(&(next->scheduling.lock), prio);
    }
}

void KeReleaseSemaphore(KeSemaphore *sem, uint32_t units)
{
    if(NULL == sem)
        return;
    PRIO queuePrio = KeAcquireSpinlock(&(KeLockingQueue.lock));
    PRIO prio = KeAcquireSpinlock(&(sem->lock));
    if(unlikely(sem->current < units))
        KePanicEx(UNACQUIRED_MUTEX_RELEASED, 2, (uintptr_t)sem, sem->current, units);
    
    sem->current -= units;
    KeSemaphoreProcessNextOnRelease(sem);
    KeReleaseSpinlock(&(sem->lock), prio);
    KeReleaseSpinlock(&(KeLockingQueue.lock), queuePrio);
}

bool KeAcquireRwLockEx(KeRwLock *rwLock, bool write, uint64_t timeout)
{
    HalCheckPriorityLevel(HAL_PRIORITY_LEVEL_PASSIVE, HAL_PRIORITY_LEVEL_PASSIVE);
    struct KeTaskControlBlock *tcb = KeGetCurrentTask();
    PRIO prio = KeAcquireSpinlock(&(rwLock->lock));
    if((0 != rwLock->writers) || (write && (0 != rwLock->readers)) || unlikely(!write && (UINT32_MAX == rwLock->readers)))
    {
        if(KE_MUTEX_NO_WAIT == timeout)
        {
            KeReleaseSpinlock(&(rwLock->lock), prio);
            return false;
        }
        else
        {
            KeBlockTask(tcb, TASK_BLOCK_MUTEX);
            PRIO tcbPrio = KeAcquireSpinlock(&(tcb->scheduling.lock));
            tcb->scheduling.block.next = NULL;
            if(NULL != rwLock->tail)
            {
                PRIO prio = KeAcquireSpinlock(&(rwLock->tail->scheduling.lock));
                rwLock->tail->scheduling.block.next = tcb;
                KeReleaseSpinlock(&(rwLock->tail->scheduling.lock), prio);
                tcb->scheduling.block.previous = rwLock->tail;
                rwLock->tail = tcb;
            }
            else
            {
                tcb->scheduling.block.previous = NULL;
                rwLock->head = tcb;
                rwLock->tail = tcb;
                rwLock->write = write;
            }
            tcb->scheduling.block.rwLock = rwLock;
            tcb->scheduling.block.acquired = false;
            tcb->scheduling.block.write = write;
            
            if(timeout < KE_MUTEX_NO_TIMEOUT)
                KeInsertToLockList(tcb, timeout);
            else
                tcb->scheduling.block.timeout.until = 0;
            KeReleaseSpinlock(&(tcb->scheduling.lock), tcbPrio);
            KeReleaseSpinlock(&(rwLock->lock), prio);
            KeTaskYield(); //suspend task and wait for an event

            return tcb->scheduling.block.acquired;
        }
    }
    else
    {
        if(write)
            rwLock->writers = 1;
        else
            rwLock->readers++;
        KeReleaseSpinlock(&(rwLock->lock), prio);
        return true;
    }
}

static inline void KeRwLockProcessNextOnRelease(KeRwLock *rwLock)
{
    while(!rwLock->write || (0 == rwLock->readers))
    {
        struct KeTaskControlBlock *next = rwLock->head;
        if(NULL == next)
            break;
        PRIO prio = KeAcquireSpinlock(&(next->scheduling.lock));

        if((next->scheduling.block.write && (0 == rwLock->readers))
            || (!next->scheduling.block.write && (0 == rwLock->writers)))
        {
            rwLock->head = next->scheduling.block.next;
            if(NULL != rwLock->head)
            {
                PRIO prio = KeAcquireSpinlock(&(rwLock->head->scheduling.lock));
                rwLock->head->scheduling.block.previous = NULL;
                rwLock->write = rwLock->head->scheduling.block.write;
                KeReleaseSpinlock(&(rwLock->head->scheduling.lock), prio);
            }
            else
            {
                rwLock->tail = NULL;
            }
            next->scheduling.block.next = NULL;
            next->scheduling.block.previous = NULL;
            next->scheduling.block.rwLock = NULL;
            next->scheduling.block.acquired = true;
            if(next->scheduling.block.write)
                rwLock->writers = 1;
            else
                rwLock->readers++;
            KeReleaseSpinlock(&(next->scheduling.lock), prio);
            if(0 != next->scheduling.block.timeout.until)
                KeRemoveFromLockList(next);
            KeUnblockTask(next);
            if(0 != rwLock->writers)
                break;
        }
        else
        {
            KeReleaseSpinlock(&(next->scheduling.lock), prio);
            break;
        }
    }
}

void KeReleaseRwLock(KeRwLock *rwLock)
{
    if(NULL == rwLock)
        return;
    PRIO queuePrio = KeAcquireSpinlock(&(KeLockingQueue.lock));
    PRIO prio = KeAcquireSpinlock(&(rwLock->lock));
    if(unlikely((0 == rwLock->readers) && (0 == rwLock->writers)))
        KePanicEx(UNACQUIRED_MUTEX_RELEASED, 3, (uintptr_t)rwLock, 0, 0);
    
    if(0 != rwLock->writers)
        rwLock->writers = 0;
    else
        rwLock->readers--;
        
    if(NULL != rwLock->head)
        KeRwLockProcessNextOnRelease(rwLock);

    KeReleaseSpinlock(&(rwLock->lock), prio);
    KeReleaseSpinlock(&(KeLockingQueue.lock), queuePrio);

}

void KeTimedExclusionRefresh(void)
{
    struct KeTaskControlBlock *s = NULL;
    uint64_t currentTimestamp = HalGetTimestamp();

    if(currentTimestamp < __atomic_load_n(&(KeLockingQueue.earliest), __ATOMIC_RELAXED))
        return;

    PRIO prio = KeAcquireSpinlock(&(KeLockingQueue.lock));
    while(NULL != KeLockingQueue.head)
    {
        s = KeLockingQueue.head;
        PRIO prio = KeAcquireSpinlock(&(s->scheduling.lock));
        if(currentTimestamp >= s->scheduling.block.timeout.until)
        {
            KeRemoveFromLockList(s);
            s->scheduling.block.acquired = false;
            if(NULL != s->scheduling.block.mutex)
            {
                KeMutex *mutex = s->scheduling.block.mutex;
                PRIO prio = KeAcquireSpinlock(&(mutex->lock));
                if(NULL != s->scheduling.block.previous)
                    s->scheduling.block.previous->scheduling.next = s->scheduling.block.next;
                else
                    mutex->head = s->scheduling.block.next;

                if(NULL != s->scheduling.block.next)
                    s->scheduling.block.next->scheduling.block.previous = s->scheduling.block.previous;
                else
                    mutex->tail = s->scheduling.block.previous;
                
                s->scheduling.block.mutex = NULL;
                KeReleaseSpinlock(&(mutex->lock), prio);
            }
            else if(NULL != s->scheduling.block.semaphore)
            {
                KeSemaphore *semaphore = s->scheduling.block.semaphore;
                PRIO prio = KeAcquireSpinlock(&(semaphore->lock));

                if(NULL != s->scheduling.block.next)
                    s->scheduling.block.next->scheduling.block.previous = s->scheduling.block.previous;
                else
                    s->scheduling.block.semaphore->tail = s->scheduling.block.previous;

                if(NULL != s->scheduling.block.previous)
                    s->scheduling.block.previous->scheduling.next = s->scheduling.block.next;
                else
                {
                    semaphore->head = s->scheduling.block.next;
                    if(NULL != semaphore->head)
                    {
                        semaphore->needed = semaphore->head->scheduling.block.count;
                        KeSemaphoreProcessNextOnRelease(semaphore);
                    }
                }

                s->scheduling.block.semaphore = NULL;
                KeReleaseSpinlock(&(semaphore->lock), prio);
            }
            else
            {
                KeRwLock *rwLock = s->scheduling.block.rwLock;
                PRIO prio = KeAcquireSpinlock(&(rwLock->lock));

                if(NULL != s->scheduling.block.next)
                    s->scheduling.block.next->scheduling.block.previous = s->scheduling.block.previous;
                else
                    s->scheduling.block.rwLock->tail = s->scheduling.block.previous;

                if(NULL != s->scheduling.block.previous)
                    s->scheduling.block.previous->scheduling.next = s->scheduling.block.next;
                else
                {
                    rwLock->head = s->scheduling.block.next;
                    if(NULL != rwLock->head)
                    {
                        rwLock->write = rwLock->head->scheduling.block.write;
                        KeRwLockProcessNextOnRelease(rwLock);
                    }
                }

                s->scheduling.block.rwLock = NULL;
                KeReleaseSpinlock(&(s->scheduling.block.rwLock->lock), prio);
            }
            KeReleaseSpinlock(&(s->scheduling.lock), prio);
            KeUnblockTask(s);
        }
        else
        {
            KeReleaseSpinlock(&(s->scheduling.lock), prio);
            break;
        }
    }
    KeReleaseSpinlock(&(KeLockingQueue.lock), prio);
}


KeMutex *KeCreateMutex(void)
{
    KeMutex *m = MmAllocateKernelHeap(sizeof(*m));
    if(NULL != m)
        *m = (KeMutex)KeMutexInitializer;
    return m;
}

KeSpinlock *KeCreateSpinlock(void)
{
    KeSpinlock *m = MmAllocateKernelHeap(sizeof(*m));
    if(NULL != m)
        *m = (KeSpinlock)KeSpinlockInitializer;
    return m;
}

KeSemaphore *KeCreateSemaphore(uint32_t initial, uint32_t max)
{
    KeSemaphore *m = MmAllocateKernelHeap(sizeof(*m));
    if(NULL != m)
    {
        *m = (KeSemaphore)KeSemaphoreInitializer;
        m->current = initial;
        m->max = max;
    }
    return m;
}

KeRwLock *KeCreateRwLock(void)
{
    KeRwLock *m = MmAllocateKernelHeap(sizeof(*m));
    if(NULL != m)
    {
        *m = (KeRwLock)KeRwLockInitializer;
    }
    return m;
}

void KeDestroyMutex(KeMutex *mutex)
{
    MmFreeKernelHeap(mutex);
}

void KeDestroySpinlock(KeSpinlock *spinlock)
{
    MmFreeKernelHeap(spinlock);
}

void KeDestroySempahore(KeSemaphore *semaphore)
{
    MmFreeKernelHeap(semaphore);
}

void KeDestroyRwLock(KeRwLock *rwLock)
{
    MmFreeKernelHeap(rwLock);
}