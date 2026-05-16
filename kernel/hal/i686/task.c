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
#include "rtl/stdlib.h"
#include "ex/load.h"
#include "hal/math.h"
#include "hal/arch.h"
#include "mm/tmem.h"
#include "hal/mm.h"

#define I686_KERNEL_STACK_SIZE 0x2000 //8 KiB

#define I686_USER_SPACE_TOP (HAL_KERNEL_SPACE_BASE - PAGE_SIZE)
#define I686_USER_STACK_DEFAULT_BASE I686_USER_SPACE_TOP
#define I686_USER_STACK_MAX_SIZE (0x1000000) //16 MiB 
#define I686_USER_STACK_DEFAULT_SIZE 0x8000 //32 KiB

#define I686_EFLAGS_IF (1 << 9) //interrupt flag
#define I686_EFLAGS_RESERVED (1 << 1) //reserved EFLAGS bits
#define I686_EFLAGS_IOPL_USER (3 << 12) //user mode EFLAGS bits

[[noreturn]] void I686StartUserTask(uint16_t ss, reg_t esp, uint16_t cs, void (*entry)(void*));

[[noreturn]] static void I686ProcessBootstrap(void (*entry)(void*), void *context, void *userStack);

STATUS HalCreateThread(struct KeProcessControlBlock *pcb, uint32_t flags,
    void (*entry)(void*), void *entryContext, void *userStack, struct KeTaskControlBlock **tcb)
{
    STATUS status = OK;
    uint32_t *stack = NULL;
    *tcb = NULL;

    *tcb = KePrepareTCB(flags);
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

STATUS HalCreateProcess(const char *path, PrivilegeLevel pl, uint32_t flags,
    void (*entry)(void*), void *entryContext, struct KeTaskControlBlock **tcb)
{
    STATUS status = OK;
    PADDRESS cr3 = 0;
    *tcb = NULL;
    struct KeProcessControlBlock *pcb = NULL;

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

    status = HalCreateThread(pcb, flags, entry, entryContext, NULL, tcb);
    if(OK == status)
    {
        (*tcb)->main = true;
        return OK;
    }

HalCreateProcessExit:
    if(NULL != pcb->data.userMemoryLock)
        KeDestroySpinlock(pcb->data.userMemoryLock);
    HalDestroyMemorySpace(pcb);
    KeDestroyPCB(pcb);

    return status;
}

STATUS HalDestroyTask(struct KeTaskControlBlock *tcb)
{
    MmFreeKernelHeap((void*)((uintptr_t)tcb->stack.kernel.top - tcb->stack.kernel.size));
    HalDestroyMathStateBuffer(tcb->data.fpu);
    KeDestroyTCB(tcb);
    return OK;
}

STATUS HalDestroyProcess(struct KeProcessControlBlock *pcb)
{
    KeDestroySpinlock(pcb->data.userMemoryLock);
    HalDestroyMemorySpace(pcb);
    KeDestroyPCB(pcb);
    return OK;
}

void HalInitializeScheduler(void)
{
    
}

[[noreturn]] static void I686ProcessBootstrap(void (*entry)(void*), void *context, void *userStack)
{
    KeReleaseInitialSchedulingLock();
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
                MM_TASK_MEMORY_STACK | MM_TASK_MEMORY_LOCKED | MM_TASK_MEMORY_FIXED | MM_TASK_MEMORY_GROWABLE, -1, 0, 0, I686_USER_STACK_MAX_SIZE, NULL);
            //since MM_TASK_MEMORY_FIXED is used, then the stack is allocated at *alignedBase* or the function fails
            if(OK == status)
            {
                struct ExProgramData *progData = nullptr;
                status = ExLoadProcessImage(tcb->parent->path, &entry, &progData);
                if(OK == status)
                {
                    //in user mode, the main thread context (passed as an argument) should be a KeTaskArguments structure
                    //and additional program data
                    struct KeTaskArguments *args = context;
                    void *argsBuffer = NULL;
                    size_t argSize = args->size + (args->argc + 1 + args->envc) * sizeof(char*);
                    size_t progDataSize = ExGetProgramDataEntryCount(progData) * sizeof(*progData);
                    status = MmMapTaskMemory(NULL, argSize + progDataSize, 
                        MM_TASK_MEMORY_READABLE | MM_TASK_MEMORY_WRITABLE, -1, 0, 0, 0, &argsBuffer);
                    if(OK == status)
                    {
                        RtlMemcpy(&((char**)argsBuffer)[args->argc + 1 + args->envc + 1], args->data, args->size);

                        char *start = (char*)&((char**)argsBuffer)[args->argc + 1 + args->envc + 1];
                        char *end = start;
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

                        RtlMemcpy((char*)argsBuffer + argSize, progData, progDataSize);

                        stack[-1] = (uintptr_t)((char*)argsBuffer + argSize); //store progdata pointer
                        stack[-2] = (uintptr_t)((char*)argsBuffer + args->argc + 1); //store envp pointer
                        stack[-3] = (uintptr_t)argsBuffer; //store argv pointer
                        stack[-4] = args->argc;
                        stack[-5] = 0; //push false return address
                        stack -= 5;

                        MmFreeKernelHeap(args);
                    }
                    else
                        LOG(SYSLOG_ERROR, "Failed to allocate memory for entry arguments: error 0x%X", (unsigned int)status);
                }
                else
                    LOG(SYSLOG_ERROR, "Failed to load process image: error 0x%X", (unsigned int)status);
            }
            else
                LOG(SYSLOG_ERROR, "Failed to allocate user stack at 0x%p of size 0x%p: error 0x%X", (void*)alignedBase, (void*)alignedSize, (unsigned int)status);
        }
        else //a child thread
        {
            //since we are in kernel mode, we need to check if the entry point and user stack is within user space page
            //in user mode the protection mechanism will take care of illegal accesses to kernel space
            if(MmProbeUserMemory(entry, 1, MM_TASK_MEMORY_EXECUTABLE) 
                && MmProbeUserMemory((void*)((uintptr_t)userStack - sizeof(*stack)), sizeof(*stack), MM_TASK_MEMORY_READABLE | MM_TASK_MEMORY_WRITABLE))
            {
                stack = userStack;
                stack[-1] = (uintptr_t)context;
                stack[-2] = 0;
                stack -= 2;
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

            //the following function can never return
            //since we do a jump to the user stack using iret, we can't return to the kernel code (here)
            //the only way to enter the kernel from user mode is through int or syscall/sysenter
            I686StartUserTask(USER_SELECTOR(GDT_USER_DS), (uintptr_t)stack, USER_SELECTOR(GDT_USER_CS), entry);   
        }
    }
    else
        entry(context);
    
    //if we are here, then this is a kernel thread and it exited
    KeFinishCurrentTask((OK == status) ? 0 : -1);
    
    while(1)
        ;
}

#endif