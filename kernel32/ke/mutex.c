#include "mutex.h"
#include "sched.h"
#include "panic.h"

void KeAcquireSpinlock(KeSpinLock_t *spinlock)
{
    KeSchedulerIncrementPostponeCounter(); //postpone task switchtes
#ifndef SMP //uniprocessor
    //spinlock is not actually needed
    //the scheduler is disabled and there is only one CPU,
    //so this thread has the exclusive access to the resource
    //if the protected resource might be accessed by another IRQ,
    //use KeAcquireSpinlockDisableIRQ()
#else //multiprocessor
    //UNTESTED!!!!
    uint16_t *lock = &(spinlock->lock);
    asm volatile("lock bts WORD [%0],0" : "=m" (lock) : ); //store bit 0 value in CF and set bit 0 (atomic)
    asm volatile("jnc .acquired"); //if bit 0 was clear previously then the lock is acquired

    asm volatile(".retry:");
    asm volatile("pause");
    asm volatile("bt WORD [%0],0" : : "m" (lock) ); //check if bit is set
    asm volatile("jc .retry"); //if so, wait for the bit to be clear

    asm volatile("lock bts WORD [%0],0" : "=m" (lock) : ); //store bit 0 value in CF and set bit 0 (atomic)
    asm volatile("jc .retry"); //if bit 0 was set previously then retry

    asm volatile(".acquired:");
#endif
}

void KeAcquireSpinlockDisableIRQ(KeSpinLock_t *spinlock)
{
    KeSchedulerIncrementPostponeCounter(); //postpone task switchtes

#ifndef SMP
    ASM("pushfd");
    ASM("pop eax");
    ASM("and eax,0x200"); //store only IF bit state
    ASM("mov %0,eax" : "=m" (spinlock->eflags) : : "eax");
    ASM("cli");
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

void KeReleaseSpinlock(KeSpinLock_t *spinlock)
{
    KeSchedulerDecrementPostponeCounter();
#ifndef SMP
    //nothing to do here
#else
    uint16_t *lock = &(spinlock->lock);
    asm volatile("lock btr WORD [%0],0" : "=m" (lock) : ); //check if bit was clear previously
    asm volatile("jc .release");
    //if so, the lock was never acquired
    asm volatile("push %0" : : "r" (UNACQUIRED_MUTEX_RELEASED));
    asm volatile("call KePanic");
    asm volatile(".release:");
#endif
}

void KeReleaseSpinlockEnableIRQ(KeSpinLock_t *spinlock)
{
    KeSchedulerDecrementPostponeCounter();
#ifdef SMP
    uint16_t *lock = &(spinlock->lock);
    asm volatile("lock btr WORD [%0],0" : "=m" (lock) : ); //check if bit was clear previously
    asm volatile("jc .IRQrelease");
    //if so, the lock was never acquired
    asm volatile("push %0" : : "r" (UNACQUIRED_MUTEX_RELEASED));
    asm volatile("call KePanic");
    asm volatile(".IRQrelease:");
#endif
    ASM("pushfd");
    ASM("pop eax");
    ASM("mov edx,%0" : : "m" (spinlock->eflags) : "eax");
    ASM("or edx,eax");
    ASM("push edx");
    ASM("popfd");
}