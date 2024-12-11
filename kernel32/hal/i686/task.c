#if defined(__i686__)

#include "rtl/string.h"
#include "i686.h"
#include "mm/mm.h"
#include "hal/arch.h"
#include "ke/task/task.h"
#include "gdt.h"
#include "ke/core/panic.h"
#include "ke/sched/sched.h"
#include "memory.h"
#include "mm/dynmap.h"
#include "assert.h"
#include "config.h"
#include "time.h"
#include "rtl/stdlib.h"
#include "ex/load.h"
#include "hal/math.h"
#include "hal/arch.h"
#include "mm/tmem.h"

#define I686_KERNEL_STACK_SIZE 0x2000 //8 KiB

#define I686_USER_SPACE_TOP (HAL_KERNEL_SPACE_BASE - PAGE_SIZE)
#define I686_USER_STACK_DEFAULT_BASE I686_USER_SPACE_TOP
#define I686_USER_STACK_MAX_SIZE (0x1000000) //16 MiB 
#define I686_USER_STACK_DEFAULT_SIZE 0x8000 //32 KiB

#define I686_EFLAGS_IF (1 << 9) //interrupt flag
#define I686_EFLAGS_RESERVED (1 << 1) //reserved EFLAGS bits
#define I686_EFLAGS_IOPL_USER (3 << 12) //user mode EFLAGS bits

NORETURN void I686StartUserTask(uint16_t ss, uintptr_t esp, uint16_t cs, void (*entry)());

static NORETURN void I686ProcessBootstrap(void (*entry)(void*), void *context, void *userStack);

STATUS HalCreateThread(struct KeProcessControlBlock *pcb, const char *name, uint32_t flags,
    void (*entry)(void*), void *entryContext, void *userStack, struct KeTaskControlBlock **tcb)
{
    STATUS status = OK;
    uint32_t *stack = NULL;
    *tcb = NULL;

    *tcb = KePrepareTCB((NULL != name) ? name : RtlGetFileName(pcb->path), flags);
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

    RtlMemset(stack, 0, I686_KERNEL_STACK_SIZE);
    stack = (uint32_t*)((uintptr_t)stack + I686_KERNEL_STACK_SIZE);

    (*tcb)->stack.kernel.top = (void*)stack;
    (*tcb)->stack.kernel.size = I686_KERNEL_STACK_SIZE;

    (*tcb)->stack.user = NULL;

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

    stack[-1] = (uintptr_t)userStack;
    stack[-2] = (uintptr_t)entryContext;
    stack[-3] = (uintptr_t)entry;
    stack[-4] = 0; //return address for bootstrap routine - never returns
    stack[-5] = I686_EFLAGS_IF | I686_EFLAGS_RESERVED;
    stack[-6] = GDT_OFFSET(GDT_KERNEL_CS);
    stack[-7] = (uintptr_t)I686ProcessBootstrap;
    //all 7 GP register, which are zeroed
    (*tcb)->data.esp -= (14 * sizeof(*stack)); //update ESP

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
        name = RtlGetFileName(path);

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

static NORETURN void I686ProcessBootstrap(void (*entry)(void*), void *context, void *userStack)
{
    STATUS status = OK;
    //this is the very first starting point when the task is scheduled for the first time
    //if this is a kernel task, then there is nothing more needed and the user space is unused
    //if this is a user task, then we must create the stack and load the image
    //since we are in the context (and address space) of the target task, everything is much easier
    struct KeTaskControlBlock *tcb = KeGetCurrentTask();
    if(PL_USER == tcb->parent->pl)
    {
        uint32_t *stack = NULL;
        if(tcb->main)
        {
            //randomize stack base (20 bits giving 1048576 positions)
            int32_t location = RtlRandom(0, 1 << 20);
            //calculate stack base with 16 byte granularity, so that the stack starts somewhere within the 16 MiB region
            stack = (void*)(I686_USER_STACK_DEFAULT_BASE - (location * 16));
            uintptr_t alignedBase = ALIGN_UP((uintptr_t)stack, PAGE_SIZE);
            uintptr_t alignedSize = alignedBase - ALIGN_DOWN((uintptr_t)stack - I686_USER_STACK_DEFAULT_SIZE, PAGE_SIZE);

            status = MmMapTaskMemory((void*)alignedBase, alignedSize, 
                MM_TASK_MEMORY_STACK | MM_TASK_MEMORY_LOCKED | MM_TASK_MEMORY_FIXED | MM_TASK_MEMORY_GROWABLE, -1, 0, I686_USER_STACK_MAX_SIZE, NULL);
            //since MM_TASK_MEMORY_FIXED is used, then the stack is allocated at *alignedBase* or the function fails
            if(OK == status)
            {
                status = ExLoadProcessImage(tcb->parent->path, &entry);
                if(OK == status)
                {
                    //randomize dynamic memory base (9 bits giving 512 positions)
                    location = RtlRandom(0, 1 << 9);
                    KeAcquireMutex(&(tcb->parent->memory.mutex));
                    //calculate dynamic memory base with page granularity
                    tcb->parent->memory.base = (void*)(ALIGN_UP((uintptr_t)tcb->parent->memory.tail->end, PAGE_SIZE) + location * PAGE_SIZE);
                    KeReleaseMutex(&(tcb->parent->memory.mutex));

                    //in user mode, the main thread context (passed as an argument) should be a KeTaskArguments structure
                    struct KeTaskArguments *args = context;
                    void *argsBuffer = NULL;
                    status = MmMapTaskMemory(NULL, args->size + (args->argc + 1 + args->envc) * sizeof(char*), 
                        MM_TASK_MEMORY_READABLE | MM_TASK_MEMORY_WRITABLE, -1, 0, 0, &argsBuffer);
                    if(OK == status)
                    {
                        RtlMemcpy(&((char**)argsBuffer)[args->argc + 1 + args->envc + 1], args->data, args->size);

                        char *start = args->data;
                        char *end = args->data;
                        for(int i = 0; i < args->argc; i++)
                        {
                            while('\0' != *end)
                                ++end;

                            ((char**)argsBuffer)[i] = start;
                            start = ++end;
                        }
                        ((char**)argsBuffer)[args->argc] = NULL;
                        for(int i = 0; i < args->envc; i++)
                        {
                            while('\0' != *end)
                                ++end;

                            ((char**)argsBuffer)[i + args->argc + 1] = start;
                            start = ++end;
                        }
                        ((char**)argsBuffer)[args->envc + args->argc + 1] = NULL;

                        stack[-1] = (uintptr_t)(&(((char**)argsBuffer)[args->argc + 1])); //store envp pointer
                        stack[-2] = (uintptr_t)argsBuffer; //store argv pointer
                        stack[-3] = args->argc;
                        stack -= 3;
                    }
                }
            }
        }
        else //a child thread
        {
            //since we are in kernel mode, we need to check if the entry point and user stack is within user space page
            //in user mode the protection mechanism will take care of illegal accesses to kernel space
            if(IS_USER_MEMORY(ALIGN_DOWN((uintptr_t)entry, PAGE_SIZE), PAGE_SIZE)
                && IS_USER_MEMORY(ALIGN_UP((uintptr_t)userStack, PAGE_SIZE) - PAGE_SIZE, PAGE_SIZE))
            {
                stack = userStack;
                stack[-1] = (uintptr_t)context;
                stack -= 1;
            }
            else
                status = BAD_PARAMETER;
        }

        if((NULL == stack) && (OK == status))
            status = OUT_OF_RESOURCES;
        
        if(OK == status)
        {
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
    
    //if we are here, then this is a kernel thread and it exited
    
    
    while(1)
        ;
}

#endif