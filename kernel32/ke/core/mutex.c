#include "mutex.h"
#include "ke/sched/sched.h"
#include "panic.h"
#include "hal/time.h"
#include "it/it.h"
#include "hal/arch.h"

//list of tasks waiting for mutex or semaphore
//this list is sorted by earliest deadline first
static struct KeTaskControlBlock *list = NULL;
static KeSpinlock listLock = KeSpinlockInitializer;

PRIO KeAcquireSpinlock(KeSpinlock *spinlock)
{
    PRIO prio = HalRaisePriorityLevel(HAL_PRIORITY_LEVEL_EXCLUSIVE);
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

static void insertToList(struct KeTaskControlBlock *tcb)
{
    PRIO prio = KeAcquireSpinlock(&listLock);
    if(NULL == list)
        list = tcb;
    else
    {
        struct KeTaskControlBlock *s = list;
        //keep list sorted: the earliest deadline is first
        while(NULL != s->nextAux)
        {
            if((s->waitUntil <= tcb->waitUntil) && (s->nextAux->waitUntil > tcb->waitUntil))
            {
                tcb->nextAux = s->nextAux;
                break;
            }
            s = s->nextAux;
        }
        s->nextAux = tcb;
    }
    KeReleaseSpinlock(&listLock, prio);
}

static void removeFromList(struct KeTaskControlBlock *tcb)
{
    PRIO prio = KeAcquireSpinlock(&listLock);
    if(list == tcb)
        list = tcb->nextAux;
    if(tcb->previousAux)
        tcb->previousAux->nextAux = tcb->nextAux;
    if(tcb->nextAux)
        tcb->nextAux->previous = tcb->previousAux;
    KeReleaseSpinlock(&listLock, prio);   
}

void KeAcquireMutex(KeMutex *mutex)
{
    KeAcquireMutexWithTimeout(mutex, KE_MUTEX_NO_TIMEOUT);
}

bool KeAcquireMutexWithTimeout(KeMutex *mutex, uint64_t timeout)
{
    HalCheckPriorityLevel(HAL_PRIORITY_LEVEL_PASSIVE, HAL_PRIORITY_LEVEL_PASSIVE);
    struct KeTaskControlBlock *current = KeGetCurrentTask();
    PRIO prio = KeAcquireSpinlock(&(mutex->spinlock));
    if(mutex->lock)
    {
        if(KE_MUTEX_NO_WAIT == timeout)
        {
            KeReleaseSpinlock(&(mutex->spinlock), prio);
            return false;
        }
        else
        {
            KeBlockTask(current, TASK_BLOCK_MUTEX);
            if(mutex->queueTop)
            {
                mutex->queueBottom->next = current;
                current->previous = mutex->queueBottom;
            }
            else
            {
                mutex->queueTop = current;
                current->previous = NULL;
            }
            mutex->queueBottom = current;
            current->semaphore = NULL;
            current->mutex = mutex;
            current->rwLock = NULL;
            KeReleaseSpinlock(&(mutex->spinlock), prio);
            if(timeout < KE_MUTEX_NO_TIMEOUT)
                current->waitUntil = HalGetTimestamp() + timeout;
            else
                current->waitUntil = KE_MUTEX_NO_TIMEOUT;
            insertToList(current);
            KeTaskYield(); //suspend task and wait for an event

            if(NULL != current->mutex)
                return true;
            else
                return false;
        }
    }
    else
    {
        mutex->lock = 1;
        KeReleaseSpinlock(&(mutex->spinlock), prio);
        return true;
    }
}

void KeReleaseMutex(KeMutex *mutex)
{
    if(NULL == mutex)
        return;
    PRIO prio = KeAcquireSpinlock(&(mutex->spinlock));
    if(0 == mutex->lock)
        KePanicEx(UNACQUIRED_MUTEX_RELEASED, 1, (uintptr_t)mutex, 0, 0);
    if(NULL == mutex->queueTop)
    {
        mutex->lock = 0;
        KeReleaseSpinlock(&(mutex->spinlock), prio);
        return;
    }
    struct KeTaskControlBlock *next = mutex->queueTop;
    mutex->queueTop = next->next;
    if(NULL != next->next)
        next->next->previous = next->previous;
    next->next = NULL;
    next->previous = NULL;

    removeFromList(next);
    KeUnblockTask(next);
    KeReleaseSpinlock(&(mutex->spinlock), prio);
}

void KeAcquireSemaphore(KeSemaphore *sem)
{
    KeAcquireSemaphoreWithTimeout(sem, KE_MUTEX_NO_TIMEOUT);
}

bool KeAcquireSemaphoreWithTimeout(KeSemaphore *sem, uint64_t timeout)
{
    HalCheckPriorityLevel(HAL_PRIORITY_LEVEL_PASSIVE, HAL_PRIORITY_LEVEL_PASSIVE);
    struct KeTaskControlBlock *current = KeGetCurrentTask();
    PRIO prio = KeAcquireSpinlock(&(sem->spinlock));
    if(sem->current == sem->max)
    {
        if(KE_MUTEX_NO_WAIT == timeout)
        {
            KeReleaseSpinlock(&(sem->spinlock), prio);
            return false;
        }
        else
        {
            KeBlockTask(current, TASK_BLOCK_MUTEX);
            if(sem->queueTop)
            {
                sem->queueBottom->next = current;
                current->previous = sem->queueBottom;
            }
            else
            {
                sem->queueTop = current;
                current->previous = NULL;
            }
            sem->queueBottom = current;
            current->semaphore = sem;
            current->mutex = NULL;
            current->rwLock = NULL;
            KeReleaseSpinlock(&(sem->spinlock), prio);
            if(timeout < KE_MUTEX_NO_TIMEOUT)
                current->waitUntil = HalGetTimestamp() + timeout;
            else
                current->waitUntil = KE_MUTEX_NO_TIMEOUT;
            insertToList(current);
            KeTaskYield(); //suspend task and wait for an event

            if(NULL != current->semaphore)
            {
                return true;
            }
            else
                return false;
        }
    }
    else
    {
        sem->current++;
        KeReleaseSpinlock(&(sem->spinlock), prio);
        return true;
    }
}

void KeReleaseSemaphore(KeSemaphore *sem)
{
    PRIO prio = KeAcquireSpinlock(&(sem->spinlock));
    if(0 == sem->current)
        KePanicEx(UNACQUIRED_MUTEX_RELEASED, 2, (uintptr_t)sem, (uintptr_t)sem->max, 0);

    if(NULL == sem->queueTop)
    {
        sem->current--;
        KeReleaseSpinlock(&(sem->spinlock), prio);
        return;
    }
    struct KeTaskControlBlock *next = sem->queueTop;
    sem->queueTop = next->next;
    if(NULL != next->next)
        next->next->previous = next->previous;
    next->next = NULL;
    next->previous = NULL;

    removeFromList(next);
    KeUnblockTask(next);
    KeReleaseSpinlock(&(sem->spinlock), prio);   
}

void KeTimedExclusionRefresh(void)
{
    struct KeTaskControlBlock *s = NULL;
    uint64_t currentTimestamp = HalGetTimestamp();
    PRIO prio = KeAcquireSpinlock(&listLock);
    while(NULL != list)
    {
        s = list;
        if(currentTimestamp >= s->waitUntil)
        {
            s->waitUntil = 0;
            list = s->nextAux;
            s->nextAux = NULL;
            s->previousAux = NULL;
            if(NULL != s->mutex)
            {
                PRIO prio = KeAcquireSpinlock(&(s->mutex->spinlock));
                if(s->previous)
                    s->previous->next = s->next;
                else
                    s->mutex->queueTop = s->next;

                if(s->next)
                    s->next->previous = s->previous;
                else
                    s->mutex->queueBottom = s->previous;

                KeReleaseSpinlock(&(s->mutex->spinlock), prio);
            }
            else if(s->semaphore)
            {
                PRIO prio = KeAcquireSpinlock(&(s->semaphore->spinlock));
                if(s->previous)
                    s->previous->next = s->next;
                else
                    s->semaphore->queueTop = s->next;

                if(s->next)
                    s->next->previous = s->previous;
                else
                    s->semaphore->queueBottom = s->previous;

                KeReleaseSpinlock(&(s->semaphore->spinlock), prio);
            }
            else
            {
                PRIO prio = KeAcquireSpinlock(&(s->rwLock->lock));
                if(s->previous)
                    s->previous->next = s->next;
                else
                    s->rwLock->queueTop = s->next;

                if(s->next)
                    s->next->previous = s->previous;
                else
                    s->rwLock->queueBottom = s->previous;

                KeReleaseSpinlock(&(s->rwLock->lock), prio);
            }
            s->mutex = NULL;
            s->semaphore = NULL;
            KeUnblockTask(s);
        }
        else
            break;
    }
    KeReleaseSpinlock(&listLock, prio);
}

bool KeAcquireRwLockWithTimeout(KeRwLock *rwLock, bool write, uint64_t timeout)
{
    HalCheckPriorityLevel(HAL_PRIORITY_LEVEL_PASSIVE, HAL_PRIORITY_LEVEL_PASSIVE);
    struct KeTaskControlBlock *current = KeGetCurrentTask();
    PRIO prio = KeAcquireSpinlock(&(rwLock->lock));
    if(rwLock->writers || (write && rwLock->readers))
    {
        if(KE_MUTEX_NO_WAIT == timeout)
        {
            KeReleaseSpinlock(&(rwLock->lock), prio);
            return false;
        }
        else
        {
            
            KeBlockTask(current, TASK_BLOCK_MUTEX);
            if(rwLock->queueTop)
            {
                rwLock->queueBottom->next = current;
                current->previous = rwLock->queueBottom;
            }
            else
            {
                rwLock->queueTop = current;
                current->previous = NULL;
            }
            rwLock->queueBottom = current;
            current->semaphore = NULL;
            current->mutex = NULL;
            current->rwLock = rwLock;
            current->blockParameters.rwLock.write = write;
            KeReleaseSpinlock(&(rwLock->lock), prio);
            if(timeout < KE_MUTEX_NO_TIMEOUT)
                    current->waitUntil = HalGetTimestamp() + timeout;
                else
                    current->waitUntil = KE_MUTEX_NO_TIMEOUT;
            insertToList(current);
            
            KeTaskYield(); //suspend task and wait for an event
        }
    }
    else
    {
        if(write)
            rwLock->writers = 1;
        else
            rwLock->readers++;

        KeReleaseSpinlock(&(rwLock->lock), prio);
    }
    return true;
}

void KeAcquireRwLock(KeRwLock *rwLock, bool write)
{
    KeAcquireRwLockWithTimeout(rwLock, write, KE_MUTEX_NO_TIMEOUT);
}

void KeReleaseRwLock(KeRwLock *rwLock)
{
    PRIO prio = KeAcquireSpinlock(&(rwLock->lock));

    if((0 == rwLock->writers) && (0 == rwLock->readers))
    {
        KeReleaseSpinlock(&(rwLock->lock), prio);
        KePanicEx(UNACQUIRED_MUTEX_RELEASED, 3, (uintptr_t)rwLock, 0, 0);
    }

    if(rwLock->readers)
        rwLock->readers--;
    else
        rwLock->writers = 0;

    if((0 == rwLock->writers) && (0 == rwLock->readers))
    {
        struct KeTaskControlBlock *t = rwLock->queueTop;
        while(NULL != t)
        {
            
            if(t->blockParameters.rwLock.write)
            {
                if(0 == rwLock->readers)
                {
                    rwLock->writers = 1;
                    rwLock->queueTop = t->next;
                    if(NULL != t->next)
                        t->next->previous = NULL;
                    else
                        rwLock->queueBottom = NULL;
                    
                    
                    KeReleaseSpinlock(&(rwLock->lock), prio);
                    KeUnblockTask(t);
                    return;
                }
                else
                {
                    KeReleaseSpinlock(&(rwLock->lock), prio);
                    return;
                }
            }

            rwLock->readers++;

            if(NULL != t->next)
                t->next->previous = NULL;
            else
                rwLock->queueBottom = NULL;

            rwLock->queueTop = t->next;
            removeFromList(t);
            
            KeUnblockTask(t);
            t = rwLock->queueTop;
        }
    }
    KeReleaseSpinlock(&(rwLock->lock), prio);
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

KeSemaphore *KeCreateSemaphore(void)
{
    KeSemaphore *m = MmAllocateKernelHeap(sizeof(*m));
    if(NULL != m)
        *m = (KeSemaphore)KeSemaphoreInitializer;
    return m;
}

KeRwLock *KeCreateRwLock(void)
{
    KeRwLock *m = MmAllocateKernelHeap(sizeof(*m));
    if(NULL != m)
        *m = (KeRwLock)KeRwLockInitializer;
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