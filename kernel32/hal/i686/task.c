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

#define I686_KERNEL_STACK_SPACE_START (MM_KERNEL_SPACE_START_ADDRESS)
#define I686_KERNEL_STACK_SPACE_SIZE 0x100000 //1 MiB
#define I686_SINGLE_KERNEL_STACK_SPACE_SIZE ALIGN_DOWN(I686_KERNEL_STACK_SPACE_SIZE / MAX_KERNEL_MODE_THREADS, MM_PAGE_SIZE)
#define I686_SINGLE_KERNEL_MAX_STACK_SIZE (I686_SINGLE_KERNEL_STACK_SPACE_SIZE - MM_PAGE_SIZE)

#define I686_USER_SPACE_TOP (MM_KERNEL_SPACE_START_ADDRESS - MM_PAGE_SIZE)
#define I686_USER_STACK_DEFAULT_BASE I686_USER_SPACE_TOP
#define I686_USER_STACK_MAX_SIZE (0x1000000 - MM_PAGE_SIZE) //16 MiB - 4 KiB (guard page)
#define I686_USER_STACK_DEFAULT_SIZE 0x8000 //32 KiB

#define I686_EFLAGS_IF (1 << 9) //interrupt flag
#define I686_EFLAGS_RESERVED (1 << 1) //reserved EFLAGS bits
#define I686_EFLAGS_IOPL_USER (3 << 12) //user mode EFLAGS bits

NORETURN void I686StartUserTask(uint16_t ss, uintptr_t esp, uint16_t cs, void (*entry)());

static NORETURN void I686ProcessBootstrap(void (*entry)(void*), void *context);

STATUS HalCreateThread(struct KeTaskControlBlock *parent, const char *name,
    void (*entry)(void*), void *entryContext, struct KeTaskControlBlock **tcb)
{
    STATUS status = OK;
    uint32_t *stack = NULL;
    *tcb = NULL;
    PRIO prio;

    if(KE_TASK_TYPE_THREAD == parent->type) //routine called for a thread, get parent process
        parent = parent->parent;

    prio = ObLockObject(parent);

    if(MAX_KERNEL_MODE_THREADS == parent->taskCount)
    {
        status = KE_KERNEL_THREAD_LIMIT_REACHED;
        goto HalCreateThreadExit;
    }

    *tcb = KePrepareTCB(parent->pl, (NULL != name) ? name : parent->name, NULL);
    if(NULL == *tcb)
    {
        status = OUT_OF_RESOURCES;
        goto HalCreateThreadExit;
    }

    (*tcb)->type = KE_TASK_TYPE_THREAD;
    (*tcb)->cpu.userMemoryLock = parent->cpu.userMemoryLock;

    (*tcb)->threadId = parent->freeTaskIds[MAX_KERNEL_MODE_THREADS - parent->taskCount - 1];
    parent->taskCount++;
    (*tcb)->parent = parent;
    
    if(NULL == parent->child)
        parent->child = *tcb;
    else
    {
        struct KeTaskControlBlock *t = parent->child;
        while(NULL != t->sibling)
            t = t->sibling;
        
        t->sibling = *tcb;
    }

    if(KeGetCurrentTaskParent() == parent)
    {
        status = MmAllocateMemory(I686_KERNEL_STACK_SPACE_START + 
            ((*tcb)->threadId + 1) * I686_SINGLE_KERNEL_STACK_SPACE_SIZE - MM_PAGE_SIZE, MM_PAGE_SIZE, MM_FLAG_WRITABLE);
        
        stack = (void*)(I686_KERNEL_STACK_SPACE_START + 
            ((*tcb)->threadId + 1) * I686_SINGLE_KERNEL_STACK_SPACE_SIZE);
    }
    else
    {
        status = I686CreateThreadKernelStack(parent, I686_KERNEL_STACK_SPACE_START + 
            ((*tcb)->threadId + 1) * I686_SINGLE_KERNEL_STACK_SPACE_SIZE, (void**)&stack);
    }
    if(OK != status)
        goto HalCreateThreadExit;
    
    CmMemset((void*)((uintptr_t)stack - MM_PAGE_SIZE), 0, MM_PAGE_SIZE);

    (*tcb)->cpu.cr3 = parent->cpu.cr3;
    (*tcb)->cpu.esp0 = (uintptr_t)(I686_KERNEL_STACK_SPACE_START + 
        ((*tcb)->threadId + 1) * I686_SINGLE_KERNEL_STACK_SPACE_SIZE);

    (*tcb)->cpu.esp = (*tcb)->cpu.esp0;
    
    (*tcb)->cpu.ds = GDT_OFFSET(GDT_KERNEL_DS);
    (*tcb)->cpu.es = (*tcb)->cpu.ds;
    (*tcb)->cpu.fs = (*tcb)->cpu.ds;
    (*tcb)->cpu.gs = (*tcb)->cpu.ds;

    (*tcb)->userStackSize = 0;
    (*tcb)->kernelStackSize = MM_PAGE_SIZE;

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
    (*tcb)->cpu.esp -= (13 * sizeof(uint32_t)); //update ESP

    if(KeGetCurrentTaskParent() != parent)
        MmUnmapDynamicMemory((void*)((uintptr_t)stack - MM_PAGE_SIZE));
    
    ObUnlockObject(parent, prio);

    return OK;

HalCreateThreadExit:
    if(NULL != *tcb)
    {
        if(0 != (*tcb)->threadId)
        {
            parent->taskCount--;
            parent->freeTaskIds[MAX_KERNEL_MODE_THREADS - parent->taskCount - 1] = (*tcb)->threadId;
            if((*tcb) == parent->child)
            {
                parent->child = (*tcb)->sibling;
            }
            else
            {
                struct KeTaskControlBlock *t = parent->child;
                while((*tcb) != t->sibling)
                    t = t->sibling;
                
                t->sibling = (*tcb)->sibling;
            }
        }
        if(NULL != (*tcb)->cpu.userMemoryLock)
            KeDestroySpinlock((*tcb)->cpu.userMemoryLock);
    }   
    KeDestroyTCB(*tcb);
    *tcb = NULL;

    ObUnlockObject(parent, prio);

    return status;
}

STATUS HalCreateProcess(const char *name, const char *path, PrivilegeLevel pl, 
    void (*entry)(void*), void *entryContext, struct KeTaskControlBlock **tcb)
{
    STATUS status = OK;
    PADDRESS cr3 = 0;
    uint32_t *stack = NULL;
    *tcb = NULL;

    if((NULL == name) && (NULL != path))
        name = CmGetFileName(path);
        
    *tcb = KePrepareTCB(pl, name, path);
    if(NULL == *tcb)
    {
        status = OUT_OF_RESOURCES;
        goto HalCreateProcessExit;
    }

    (*tcb)->type = KE_TASK_TYPE_PROCESS;
    (*tcb)->cpu.userMemoryLock = KeCreateSpinlock();
    if(NULL == (*tcb)->cpu.userMemoryLock)
    {
        status = OUT_OF_RESOURCES;
        goto HalCreateProcessExit;
    }

    //TODO: randomize kernel stack location?
    status = I686CreateProcessMemorySpace(&cr3, I686_KERNEL_STACK_SPACE_START + I686_SINGLE_KERNEL_STACK_SPACE_SIZE, (void*)&stack);
    if(OK != status)
        goto HalCreateProcessExit;
    
    CmMemset((void*)((uintptr_t)stack - MM_PAGE_SIZE), 0, MM_PAGE_SIZE);

    (*tcb)->cpu.cr3 = cr3;
    (*tcb)->cpu.esp0 = I686_KERNEL_STACK_SPACE_START + I686_SINGLE_KERNEL_STACK_SPACE_SIZE;
    (*tcb)->cpu.esp = (*tcb)->cpu.esp0;
    
    (*tcb)->cpu.ds = GDT_OFFSET(GDT_KERNEL_DS);
    (*tcb)->cpu.es = (*tcb)->cpu.ds;
    (*tcb)->cpu.fs = (*tcb)->cpu.ds;
    (*tcb)->cpu.gs = (*tcb)->cpu.ds;

    (*tcb)->kernelStackSize = MM_PAGE_SIZE;
    (*tcb)->kernelStackTop = (void*)(I686_KERNEL_STACK_SPACE_START + I686_SINGLE_KERNEL_STACK_SPACE_SIZE);
    
    (*tcb)->taskCount = 1;
    (*tcb)->threadId = 0;

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
    (*tcb)->cpu.esp -= (13 * sizeof(uint32_t)); //update ESP

    MmUnmapDynamicMemory((void*)((uintptr_t)stack - MM_PAGE_SIZE));
    return OK;

HalCreateProcessExit:
    if(NULL != (*tcb)->cpu.userMemoryLock)
        KeDestroySpinlock((*tcb)->cpu.userMemoryLock);
    KeDestroyTCB(*tcb);
    *tcb = NULL;

    return status;
}


void HalInitializeScheduler(void)
{
    I686NotifyLapicTimerStarted();
}

STATUS HalAttachToTask(struct KeTaskControlBlock *target)
{
    //struct KeTaskControlBlock *current = KeGetCurrentTask();

    return OK;
}

static NORETURN void I686ProcessBootstrap(void (*entry)(void*), void *context)
{
    STATUS status = OK;
    //this is the very first starting point when the task is scheduled for the first time
    //if this is a kernel task, then there is nothing more needed and the user space is unused
    //if this is a user task, then we must create the stack and load the image
    //since we are in the context (and address space) of the target task, everything is much easier
    struct KeTaskControlBlock *tcb = KeGetCurrentTask();
    if(PL_USER == tcb->pl)
    {
        //randomize stack base (20 bits giving 1048576 positions)
        int32_t location = RtlRandom(0, 1 << 20);
        //calculate stack base with 16 byte granularity, so that the stack the stack is somewhere within the 16 MiB region
        if(KE_TASK_TYPE_THREAD == tcb->type)
            tcb->userStackTop = (void*)(I686_USER_STACK_DEFAULT_BASE 
                - (2 * (I686_USER_STACK_MAX_SIZE + MM_PAGE_SIZE) * (tcb->threadId + 1)) - (location * 16));
        else //KE_TASK_TYPE_PROCESS
            tcb->userStackTop = (void*)(I686_USER_STACK_DEFAULT_BASE - (location * 16));
        uintptr_t alignedBase = ALIGN_DOWN((uintptr_t)tcb->userStackTop - I686_USER_STACK_DEFAULT_SIZE, MM_PAGE_SIZE);
        uintptr_t alignedSize = ALIGN_UP((uintptr_t)tcb->userStackTop, MM_PAGE_SIZE) - alignedBase;
        tcb->userStackSize = (uintptr_t)tcb->userStackTop - alignedBase;

        //there is a risk that the stack might not fit
        //in such a case, the function below should fail almost immediately, since the memory is allocated from the base
        status = MmAllocateMemory(alignedBase, alignedSize, MM_FLAG_USER_MODE | MM_FLAG_WRITABLE);
        if(OK == status)
        {
            uint32_t *stack = (uint32_t*)tcb->userStackTop;
            //TODO: place entry arguments on user stack

            if(KE_TASK_TYPE_PROCESS == tcb->type)
            {
                status = ExLoadProcessImage(tcb->path, &entry);
            }

            if(OK == status)
            {
                if(KE_TASK_TYPE_PROCESS == tcb->type)
                {
                    struct KeTaskControlBlock *t;
                    KeCreateThread(tcb, NULL, (void(*)(void*))0x8048080, NULL, &t);
                    KeEnableTask(t);
                }

                tcb->cpu.ds = GDT_OFFSET(GDT_USER_DS) | 0x3;
                tcb->cpu.es = tcb->cpu.ds;
                tcb->cpu.fs = tcb->cpu.ds;
                tcb->cpu.gs = tcb->cpu.ds;

                //the following function can never return
                //since we do a jump to the user stack using iret, we can't return to the kernel code (here)
                //the only way to enter the kernel from user mode is through int or syscall/sysenter
                I686StartUserTask(GDT_OFFSET(GDT_USER_DS) | 0x3, (uintptr_t)stack, GDT_OFFSET(GDT_USER_CS) | 0x3, entry);   
            } //TODO: else terminate
        }
    }
    else
        entry(context);
    
    
    while(1)
        ;
}

#endif