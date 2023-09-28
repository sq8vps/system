#include "mutex.h"
#include "ke/sched/sched.h"
#include "panic.h"
#include "hal/time.h"

//list of tasks waiting for mutex or semaphore
//this list is sorted by earliest deadline first
static struct KeTaskControlBlock *list = NULL;
static KeSpinlock listLock = KeSpinlockInitializer;

void KeAcquireSpinlock(KeSpinlock *spinlock)
{
    KeSchedulerIncrementPostponeCounter(); //postpone task switchtes

#ifndef SMP
    ASM("pushfd");
    ASM("pop eax");
    ASM("and eax,0x200"); //store only the IF bit state
    ASM("mov %0,eax" : "=m" (spinlock->eflags) : : "eax", "memory");
    ASM("cli");
    if(spinlock->lock)
        KePanicEx(BUSY_MUTEX_ACQUIRED, (uintptr_t)spinlock, 0, 0, 0);
    spinlock->lock = 1;
#else
    //UNTESTED!!!!
    uint16_t *lock = &(spinlock->lock);
    uint32_t *eflags = &(spinlock->eflags);
    asm volatile("pushfd"); //store flags
    asm volatile("cli");
    asm volatile("lock bts WORD [%0],0" : "=m" (lock) : ); //store bit 0 value in CF and set bit 0 (atomic)
    asm volatile("jnc .IRQacquired"); //if bit 0 was clear previously then the lock is acquired

    asm volatile(".IRQretry:");
    asm volatile("popfd"); //restore previous flags
    asm volatile("pushfd"); //store previous flags again
    asm volatile("pause");
    asm volatile("bt WORD [%0],0" : : "m" (lock)); //check if bit is set
    asm volatile("jc .IRQretry"); //if so, wait for the bit to be clear

    asm volatile("cli");
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
    ASM("pushfd" : : : "memory");
    ASM("pop eax" : : : "eax", "memory");
    ASM("mov edx,%0" : : "m" (spinlock->eflags) : "eax", "edx", "memory");
    ASM("or edx,eax" : : : "edx", "eax");
    ASM("push edx" : : : "edx", "memory");
    ASM("popfd" : : : "flags", "memory");
    KeSchedulerDecrementPostponeCounter();
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
            current->timedExclusion.mutex = mutex;
            KeReleaseSpinlock(&(mutex->spinlock));
            if(timeout < KE_MUTEX_NORMAL)
                current->waitUntil = HalGetTimestamp() + timeout;
            else
                current->waitUntil = KE_MUTEX_NORMAL;
            insertToList(current);
            KeTaskYield(); //suspend task and wait for an event

            if(NULL != current->timedExclusion.mutex)
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
            current->timedExclusion.semaphore = sem;
            KeReleaseSpinlock(&(sem->spinlock));
            if(timeout < KE_MUTEX_NORMAL)
                current->waitUntil = HalGetTimestamp() + timeout;
            else
                current->waitUntil = KE_MUTEX_NORMAL;
            insertToList(current);
            KeTaskYield(); //suspend task and wait for an event

            if(NULL != current->timedExclusion.semaphore)
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
            KeAcquireSpinlock(&(s->timedExclusion.mutex->spinlock));
            if(s->previous)
                s->previous->next = s->next;
            else
                s->timedExclusion.mutex->queueTop = s->next;

            if(s->next)
                s->next->previous = s->previous;
            else
                s->timedExclusion.mutex->queueBottom = s->previous;

            KeReleaseSpinlock(&(s->timedExclusion.mutex->spinlock));    
            s->timedExclusion.mutex = NULL;
            KeUnblockTask(s);
        }
        else
            break;
    }
    KeReleaseSpinlock(&listLock);
}