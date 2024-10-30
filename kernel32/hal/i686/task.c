#if defined(__i686__)

#include "i686.h"
#include "mm/mm.h"
#include "hal/arch.h"
#include "ke/task/task.h"
#include "gdt.h"
#include "ke/core/panic.h"
#include "ke/sched/sched.h"
#include "memory.h"
#include "common.h"
#include "mm/dynmap.h"
#include "assert.h"
#include "config.h"
#include "time.h"
#include "rtl/random.h"
#include "ex/load.h"
#include "hal/math.h"

#define I686_KERNEL_STACK_SIZE 0x2000 //8 KiB

#define I686_USER_SPACE_TOP (MM_KERNEL_SPACE_START_ADDRESS - MM_PAGE_SIZE)
#define I686_USER_STACK_DEFAULT_BASE I686_USER_SPACE_TOP
#define I686_USER_STACK_MAX_SIZE (0x1000000 - MM_PAGE_SIZE) //16 MiB - 4 KiB (guard page)
#define I686_USER_STACK_DEFAULT_SIZE 0x8000 //32 KiB

#define I686_EFLAGS_IF (1 << 9) //interrupt flag
#define I686_EFLAGS_RESERVED (1 << 1) //reserved EFLAGS bits
#define I686_EFLAGS_IOPL_USER (3 << 12) //user mode EFLAGS bits

NORETURN void I686StartUserTask(uint16_t ss, uintptr_t esp, uint16_t cs, void (*entry)());

static NORETURN void I686ProcessBootstrap(void (*entry)(void*), void *context);

STATUS HalCreateThread(struct KeProcessControlBlock *pcb, const char *name, uint32_t flags,
    void (*entry)(void*), void *entryContext, void *userStack, struct KeTaskControlBlock **tcb)
{
    STATUS status = OK;
    uint32_t *stack = NULL;
    *tcb = NULL;

    *tcb = KePrepareTCB((NULL != name) ? name : CmGetFileName(pcb->path), flags);
    if(NULL == *tcb)
    {
        status = OUT_OF_RESOURCES;
        goto HalCreateThreadExit;
    }

    (*tcb)->data.fpu = HalCreateMathStateBuffer();
    if(NULL == (*tcb)->data.fpu)
    {
        status = OUT_OF_RESOURCES;
        goto HalCreateThreadExit; 
    }

    status = KeAssociateTCB(pcb, *tcb);
    if(OK != status)
        goto HalCreateThreadExit;

    stack = MmAllocateKernelHeapAligned(I686_KERNEL_STACK_SIZE, 16);
    if(NULL == stack)
    {
        status = OUT_OF_RESOURCES;
        goto HalCreateThreadExit;
    }

    CmMemset(stack, 0, I686_KERNEL_STACK_SIZE);
    stack = (uint32_t*)((uintptr_t)stack + I686_KERNEL_STACK_SIZE);

    (*tcb)->stack.kernel.top = (void*)stack;
    (*tcb)->stack.kernel.size = I686_KERNEL_STACK_SIZE;

    (*tcb)->stack.user.top = userStack;
    (*tcb)->stack.user.size = 0; //FIXME: do something with this? Is this even necessary?

    (*tcb)->data.cr3 = pcb->data.cr3;
    (*tcb)->data.esp0 = (uintptr_t)((*tcb)->stack.kernel.top);
    (*tcb)->data.esp = (*tcb)->data.esp0;
    
    (*tcb)->data.ds = GDT_OFFSET(GDT_KERNEL_DS);
    (*tcb)->data.es = (*tcb)->data.ds;
    (*tcb)->data.fs = (*tcb)->data.ds;
    (*tcb)->data.gs = (*tcb)->data.ds;

    //all processes start executing by calling a fundamental bootstrap routine
    //this routine sets up stack and then calls the provided entry point
    //after the entry point routine returns (the process exits), control is returned
    //to the bootstrap routine again
    //the stack layout is as follows (top to bottom):
    //1. arguments for the bootstrap routine: entry point and entry context
    //2. architecture specific elements - these are popped by the CPU on IRET:
    //- EFLAGS - interrupt flag, reserved bits
    //- CS - privileged mode code segment
    //- EIP - process entry point
    //3. return (jump) address on returning from interrupt - this is also popped by the CPU on IRET
    //4. GP registers (7): eax, ebx, ecx, edx, esi, edi, ebp - these are popped by the context switch code
    //there is no ESP and SS, because there is no privilege level switch


    stack[-1] = (uintptr_t)entryContext;
    stack[-2] = (uintptr_t)entry;
    stack[-3] = 0; //return address for bootstrap routine - never returns
    stack[-4] = I686_EFLAGS_IF | I686_EFLAGS_RESERVED;
    stack[-5] = GDT_OFFSET(GDT_KERNEL_CS);
    stack[-6] = (uintptr_t)I686ProcessBootstrap;
    //all 7 GP register, which are zeroed
    (*tcb)->data.esp -= (13 * sizeof(uint32_t)); //update ESP

    return OK;

HalCreateThreadExit:
    MmFreeKernelHeap(stack);
    if(NULL != tcb)
        KeDissociateTCB(*tcb);
    HalDestroyMathStateBuffer((*tcb)->data.fpu);
    KeDestroyTCB(*tcb);
    *tcb = NULL;

    return status;
}

STATUS HalCreateProcess(const char *name, const char *path, PrivilegeLevel pl, uint32_t flags,
    void (*entry)(void*), void *entryContext, struct KeTaskControlBlock **tcb)
{
    STATUS status = OK;
    PADDRESS cr3 = 0;
    *tcb = NULL;
    struct KeProcessControlBlock *pcb = NULL;

    if((NULL == name) && (NULL != path))
        name = CmGetFileName(path);

    pcb = KePreparePCB(pl, path, 0);
    if(NULL == pcb)
        return OUT_OF_RESOURCES;

    pcb->data.userMemoryLock = KeCreateSpinlock();
    if(NULL == pcb->data.userMemoryLock)
    {
        status = OUT_OF_RESOURCES;
        goto HalCreateProcessExit;
    }

    cr3 = I686CreateNewMemorySpace();
    if(0 == cr3)
        goto HalCreateProcessExit;

    pcb->data.cr3 = cr3;

    status = HalCreateThread(pcb, name, flags, entry, entryContext, NULL, tcb);
    if(OK == status)
    {
        (*tcb)->main = true;
        return OK;
    }

HalCreateProcessExit:
    if(NULL != pcb->data.userMemoryLock)
        KeDestroySpinlock(pcb->data.userMemoryLock);
    KeDestroyPCB(pcb);
    I686DestroyMemorySpace(cr3);

    return status;
}


void HalInitializeScheduler(void)
{
    I686NotifyLapicTimerStarted();
}

static NORETURN void I686ProcessBootstrap(void (*entry)(void*), void *context)
{
    STATUS status = OK;
    //this is the very first starting point when the task is scheduled for the first time
    //if this is a kernel task, then there is nothing more needed and the user space is unused
    //if this is a user task, then we must create the stack and load the image
    //since we are in the context (and address space) of the target task, everything is much easier
    struct KeTaskControlBlock *tcb = KeGetCurrentTask();
    if(PL_USER == tcb->parent->pl)
    {
        uint32_t *stack = (uint32_t*)tcb->stack.user.top;
        if(tcb->main)
        {
            //randomize stack base (20 bits giving 1048576 positions)
            int32_t location = RtlRandom(0, 1 << 20);
            //calculate stack base with 16 byte granularity, so that the stack the stack is somewhere within the 16 MiB region
            tcb->stack.user.top = (void*)(I686_USER_STACK_DEFAULT_BASE - (location * 16));
            uintptr_t alignedBase = ALIGN_DOWN((uintptr_t)tcb->stack.user.top - I686_USER_STACK_DEFAULT_SIZE, MM_PAGE_SIZE);
            uintptr_t alignedSize = ALIGN_UP((uintptr_t)tcb->stack.user.top, MM_PAGE_SIZE) - alignedBase;
            tcb->stack.user.size = (uintptr_t)tcb->stack.user.top - alignedBase;

            status = MmAllocateMemory(alignedBase, alignedSize, MM_FLAG_USER_MODE | MM_FLAG_WRITABLE);
            if(OK == status)
            {
                stack = (uint32_t*)tcb->stack.user.top;
                status = ExLoadProcessImage(tcb->parent->path, &entry);
            }
        }
        else if(NULL == stack)
            status = OUT_OF_RESOURCES;
        
        if(OK == status)
        {
            //TODO: place entry arguments on user stack

            tcb->data.ds = USER_SELECTOR(GDT_USER_DS);
            tcb->data.es = tcb->data.ds;
            tcb->data.fs = tcb->data.ds;
            tcb->data.gs = tcb->data.ds;

            //the following function can never return
            //since we do a jump to the user stack using iret, we can't return to the kernel code (here)
            //the only way to enter the kernel from user mode is through int or syscall/sysenter
            I686StartUserTask(USER_SELECTOR(GDT_USER_DS), (uintptr_t)stack, USER_SELECTOR(GDT_USER_CS), entry);   
        } //TODO: else terminate
    }
    else
        entry(context);
    
    
    while(1)
        ;
}

#endif