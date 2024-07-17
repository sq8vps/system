#include "mutex.h"
#include "ke/sched/sched.h"
#include "panic.h"
#include "hal/time.h"
#include "it/it.h"

//list of tasks waiting for mutex or semaphore
//this list is sorted by earliest deadline first
static struct KeTaskControlBlock *list = NULL;
static KeSpinlock listLock = KeSpinlockInitializer;

void KeAcquireSpinlock(KeSpinlock *spinlock)
{
#ifndef SMP
    spinlock->priority = HalRaisePriorityLevel(HAL_PRIORITY_LEVEL_EXCLUSIVE);
    if(0 != spinlock->lock)
        KePanicEx(BUSY_MUTEX_ACQUIRED, (uintptr_t)spinlock, 0, 0, 0);
    spinlock->lock = 1;
#else
    //UNTESTED!!!!
    uint16_t *lock = &(spinlock->lock);
    uint32_t *eflags = &(spinlock->eflags);
    asm volatile("pushfd"); //store flags
    ItDisableInterrupts();
    asm volatile("lock bts WORD [%0],0" : "=m" (lock) : ); //store bit 0 value in CF and set bit 0 (atomic)
    asm volatile("jnc .IRQacquired"); //if bit 0 was clear previously then the lock is acquired

    asm volatile(".IRQretry:");
    asm volatile("popfd"); //restore previous flags
    asm volatile("pushfd"); //store previous flags again
    asm volatile("pause");
    asm volatile("bt WORD [%0],0" : : "m" (lock)); //check if bit is set
    asm volatile("jc .IRQretry"); //if so, wait for the bit to be clear

    ItDisableInterrupts();
    asm volatile("lock bts WORD [%0],0" : "=m" (lock) : ); //store bit 0 value in CF and set bit 0 (atomic)
    asm volatile("jc .IRQretry"); //if bit 0 was set previously then retry

    asm volatile(".IRQacquired:");
    asm volatile("pop eax");
    asm volatile("mov [%0],eax" : "=m" (eflags) : ); //store previous flags
#endif
}

void KeReleaseSpinlock(KeSpinlock *spinlock)
{
#ifndef SMP
    if(0 == spinlock->lock)
        KePanicEx(UNACQUIRED_MUTEX_RELEASED, (uintptr_t)spinlock, 0, 0, 0);
    spinlock->lock = 0;

#else
    uint16_t *lock = &(spinlock->lock);
    asm volatile("lock btr WORD [%0],0" : "=m" (lock) : ); //check if bit was clear previously
    asm volatile("jc .IRQrelease");
    //if so, the lock was never acquired
    asm volatile("push %0" : : "r" (UNACQUIRED_MUTEX_RELEASED));
    asm volatile("call KePanic");
    asm volatile(".IRQrelease:");
#endif
    HalLowerPriorityLevel(spinlock->priority);
}

static void insertToList(struct KeTaskControlBlock *tcb)
{
    KeAcquireSpinlock(&listLock);
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
    KeReleaseSpinlock(&listLock);
}

static void removeFromList(struct KeTaskControlBlock *tcb)
{
    KeAcquireSpinlock(&listLock);
    if(list == tcb)
        list = tcb->nextAux;
    if(tcb->previousAux)
        tcb->previousAux->nextAux = tcb->nextAux;
    if(tcb->nextAux)
        tcb->nextAux->previous = tcb->previousAux;
    KeReleaseSpinlock(&listLock);   
}

void KeAcquireMutex(KeMutex *mutex)
{
    KeAcquireMutexWithTimeout(mutex, KE_MUTEX_NORMAL);
}

bool KeAcquireMutexWithTimeout(KeMutex *mutex, uint64_t timeout)
{
    struct KeTaskControlBlock *current = KeGetCurrentTask();
    KeAcquireSpinlock(&(mutex->spinlock));
    if(mutex->lock)
    {
        if(KE_MUTEX_NO_WAIT == timeout)
        {
            KeReleaseSpinlock(&(mutex->spinlock));
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
            KeReleaseSpinlock(&(mutex->spinlock));
            if(timeout < KE_MUTEX_NORMAL)
                current->waitUntil = HalGetTimestamp() + timeout;
            else
                current->waitUntil = KE_MUTEX_NORMAL;
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
        KeReleaseSpinlock(&(mutex->spinlock));
        return true;
    }
}

void KeReleaseMutex(KeMutex *mutex)
{
    if(NULL == mutex)
        return;
    KeAcquireSpinlock(&(mutex->spinlock));
    if(0 == mutex->lock)
        KePanicEx(UNACQUIRED_MUTEX_RELEASED, 1, (uintptr_t)mutex, 0, 0);
    if(NULL == mutex->queueTop)
    {
        mutex->lock = 0;
        KeReleaseSpinlock(&(mutex->spinlock));
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
    KeReleaseSpinlock(&(mutex->spinlock));
}

void KeAcquireSemaphore(KeSemaphore *sem)
{
    KeAcquireSemaphoreWithTimeout(sem, KE_MUTEX_NORMAL);
}

bool KeAcquireSemaphoreWithTimeout(KeSemaphore *sem, uint64_t timeout)
{
    struct KeTaskControlBlock *current = KeGetCurrentTask();
    KeAcquireSpinlock(&(sem->spinlock));
    if(sem->current == sem->max)
    {
        if(KE_MUTEX_NO_WAIT == timeout)
        {
            KeReleaseSpinlock(&(sem->spinlock));
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
            KeReleaseSpinlock(&(sem->spinlock));
            if(timeout < KE_MUTEX_NORMAL)
                current->waitUntil = HalGetTimestamp() + timeout;
            else
                current->waitUntil = KE_MUTEX_NORMAL;
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
        KeReleaseSpinlock(&(sem->spinlock));
        return true;
    }
}

void KeReleaseSemaphore(KeSemaphore *sem)
{
    KeAcquireSpinlock(&(sem->spinlock));
    if(0 == sem->current)
        KePanicEx(UNACQUIRED_MUTEX_RELEASED, 2, (uintptr_t)sem, (uintptr_t)sem->max, 0);

    if(NULL == sem->queueTop)
    {
        sem->current--;
        KeReleaseSpinlock(&(sem->spinlock));
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
    KeReleaseSpinlock(&(sem->spinlock));   
}

void KeTimedExclusionRefresh(void)
{
    struct KeTaskControlBlock *s = NULL;
    uint64_t currentTimestamp = HalGetTimestamp();
    KeAcquireSpinlock(&listLock);
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
                KeAcquireSpinlock(&(s->mutex->spinlock));
                if(s->previous)
                    s->previous->next = s->next;
                else
                    s->mutex->queueTop = s->next;

                if(s->next)
                    s->next->previous = s->previous;
                else
                    s->mutex->queueBottom = s->previous;

                KeReleaseSpinlock(&(s->mutex->spinlock));
            }
            else if(s->semaphore)
            {
                KeAcquireSpinlock(&(s->semaphore->spinlock));
                if(s->previous)
                    s->previous->next = s->next;
                else
                    s->semaphore->queueTop = s->next;

                if(s->next)
                    s->next->previous = s->previous;
                else
                    s->semaphore->queueBottom = s->previous;

                KeReleaseSpinlock(&(s->semaphore->spinlock));
            }
            else
            {
                KeAcquireSpinlock(&(s->rwLock->lock));
                if(s->previous)
                    s->previous->next = s->next;
                else
                    s->rwLock->queueTop = s->next;

                if(s->next)
                    s->next->previous = s->previous;
                else
                    s->rwLock->queueBottom = s->previous;

                KeReleaseSpinlock(&(s->rwLock->lock));
            }
            s->mutex = NULL;
            s->semaphore = NULL;
            KeUnblockTask(s);
        }
        else
            break;
    }
    KeReleaseSpinlock(&listLock);
}

bool KeAcquireRwLockWithTimeout(KeRwLock *rwLock, bool write, uint64_t timeout)
{
    struct KeTaskControlBlock *current = KeGetCurrentTask();
    KeAcquireSpinlock(&(rwLock->lock));
    if(rwLock->writers || (write && rwLock->readers))
    {
        if(KE_MUTEX_NO_WAIT == timeout)
        {
            KeReleaseSpinlock(&(rwLock->lock));
            return false;
        }
        else
        {
            ObLockObject(current);
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
            KeReleaseSpinlock(&(rwLock->lock));
            if(timeout < KE_MUTEX_NORMAL)
                    current->waitUntil = HalGetTimestamp() + timeout;
                else
                    current->waitUntil = KE_MUTEX_NORMAL;
            insertToList(current);
            ObUnlockObject(current);
            KeTaskYield(); //suspend task and wait for an event
        }
    }
    else
    {
        if(write)
            rwLock->writers = 1;
        else
            rwLock->readers++;

        KeReleaseSpinlock(&(rwLock->lock));
    }
    return true;
}

void KeAcquireRwLock(KeRwLock *rwLock, bool write)
{
    KeAcquireRwLockWithTimeout(rwLock, write, KE_MUTEX_NORMAL);
}

void KeReleaseRwLock(KeRwLock *rwLock)
{
    KeAcquireSpinlock(&(rwLock->lock));

    if((0 == rwLock->writers) && (0 == rwLock->readers))
    {
        KeReleaseSpinlock(&(rwLock->lock));
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
            ObLockObject(t);
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
                    
                    ObUnlockObject(t);
                    KeReleaseSpinlock(&(rwLock->lock));
                    KeUnblockTask(t);
                    return;
                }
                else
                {
                    KeReleaseSpinlock(&(rwLock->lock));
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
            ObUnlockObject(t);
            KeUnblockTask(t);
            t = rwLock->queueTop;
        }
    }
    KeReleaseSpinlock(&(rwLock->lock));
}