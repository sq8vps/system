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

#define I686_KERNEL_STACK_SPACE_START (MM_KERNEL_SPACE_START_ADDRESS)
#define I686_KERNEL_STACK_SPACE_SIZE 0x100000 //1 MiB
#define I686_SINGLE_KERNEL_STACK_SPACE_SIZE ALIGN_DOWN(I686_KERNEL_STACK_SPACE_SIZE / MAX_KERNEL_MODE_THREADS, MM_PAGE_SIZE)
#define I686_SINGLE_KERNEL_MAX_STACK_SIZE (I686_SINGLE_KERNEL_STACK_SPACE_SIZE - MM_PAGE_SIZE)

#define I686_EFLAGS_IF (1 << 9) //interrupt flag
#define I686_EFLAGS_RESERVED (1 << 1) //reserved EFLAGS bits
#define I686_EFLAGS_IOPL_USER (3 << 12) //user mode EFLAGS bits

static NORETURN void I686ProcessBootstrap(void (*entry)(void*), void *context);

// STATUS HalCreateThread(struct KeTaskControlBlock *parent, const char *name, const char *path, PrivilegeLevel pl, 
//     void (*entry)(void*), void *entryContext, struct KeTaskControlBlock **tcb)
// {
//     STATUS status = OK;
//     PADDRESS cr3 = 0;
//     uint32_t *stack = NULL;
//     *tcb = NULL;
//     uintptr_t taskNumber = 0;

//     ASSERT(NULL != parent);

//     PRIO prio = ObLockObject(parent);

//     *tcb = KePrepareTCB(parent->pl, (NULL != name) ? name : parent->name, NULL);
//     if(NULL == *tcb)
//     {
//         status = OUT_OF_RESOURCES;
//         goto HalCreateProcessRawExit;
//     }



//     //TODO: randomize stack location
//     status = I686CreateThreadKernelStack(parent, I686_KERNEL_STACK_TOP, (void*)&stack);
//     if(OK != status)
//         goto HalCreateProcessRawExit;
    
//     CmMemset((void*)((uintptr_t)stack - MM_PAGE_SIZE), 0, MM_PAGE_SIZE);

//     (*tcb)->cpu.cr3 = cr3;
//     (*tcb)->cpu.esp = I686_KERNEL_STACK_TOP;
//     (*tcb)->cpu.esp0 = I686_KERNEL_STACK_TOP;
    
//     (*tcb)->cpu.ds = GDT_OFFSET(GDT_KERNEL_DS);
//     (*tcb)->cpu.es = (*tcb)->cpu.ds;
//     (*tcb)->cpu.fs = (*tcb)->cpu.ds;
//     (*tcb)->cpu.gs = (*tcb)->cpu.ds;

//     (*tcb)->userStackSize = I686_PROCESS_INITIAL_STACK_SIZE;
//     (*tcb)->kernelStackSize = I686_KERNEL_INITIAL_STACK_SIZE;

//     //all processes start executing by calling a fundamental bootstrap routine
//     //this routine sets up stack and then calls the provided entry point
//     //after the entry point routine returns (the process exits), control is returned
//     //to the bootstrap routine again
//     //the stack layout is as follows (top to bottom):
//     //1. arguments for the bootstrap routine: entry point and entry context
//     //2. architecture specific elements - these are popped by the CPU on IRET:
//     //- EFLAGS - interrupt flag, reserved bits
//     //- CS - privileged mode code segment
//     //- EIP - process entry point
//     //3. return (jump) address on returning from interrupt - this is also popped by the CPU on IRET
//     //4. GP registers (7): eax, ebx, ecx, edx, esi, edi, ebp - these are popped by the context switch code
//     //there is no ESP and SS, because there is no privilege level switch


//     stack[-1] = (uintptr_t)entryContext;
//     stack[-2] = (uintptr_t)entry;
//     stack[-3] = 0; //return address for bootstrap routine - never returns
//     stack[-4] = I686_EFLAGS_IF | I686_EFLAGS_RESERVED;
//     stack[-5] = GDT_OFFSET(GDT_KERNEL_CS);
//     stack[-6] = (uintptr_t)I686ProcessBootstrap;
//     //all 7 GP register, which are zeroed
//     (*tcb)->cpu.esp -= (13 * sizeof(uint32_t)); //update ESP

//     (*tcb)->pid = (*tcb)->tid;

//     MmUnmapDynamicMemory((void*)((uintptr_t)stack - MM_PAGE_SIZE));
//     return OK;

// HalCreateProcessRawExit:
//     if(NULL != (*tcb)->cpu.userPageTableLock)
//         KeDestroySpinlock((*tcb)->cpu.userPageTableLock);
//     KeDestroyTCB(*tcb);
//     *tcb = NULL;

//     return status;
// }

// STATUS HalCreateThread(const char *name, const char *path, 
//     void (*entry)(void*), void *entryContext, struct KeTaskControlBlock **tcb)
// {
//     STATUS status = OK;
//     uint32_t *stack = NULL;
//     *tcb = NULL;
//     struct KeTaskControlBlock *parent = NULL;
//     PRIO prio;

//     parent = KeGetCurrentTask();
//     if(NULL != parent->parent) //routine called from a thread, get parent process
//         parent = parent->parent;

//     prio = ObLockObject(parent);

//     if(MAX_KERNEL_MODE_THREADS == parent->taskCount)
//     {
//         status = KE_KERNEL_THREAD_LIMIT_REACHED;
//         goto HalCreateThreadExit;
//     }

//     *tcb = KePrepareTCB(parent->pl, (NULL != name) ? name : parent->name, NULL);
//     if(NULL == *tcb)
//     {
//         status = OUT_OF_RESOURCES;
//         goto HalCreateThreadExit;
//     }

//     (*tcb)->threadId = parent->freeTaskIds[MAX_KERNEL_MODE_THREADS - parent->taskCount - 1];
//     parent->taskCount++;

//     //allocate stack
//     MmAllocateMemory(I686_KERNEL_STACK_SPACE_START + 
//         ((*tcb)->threadId + 1) * I686_SINGLE_KERNEL_STACK_SPACE_SIZE - MM_PAGE_SIZE, 
//         MM_PAGE_SIZE, MM_FLAG_WRITABLE);
    
//     CmMemset((void*)((uintptr_t)stack - MM_PAGE_SIZE), 0, MM_PAGE_SIZE);

//     (*tcb)->cpu.cr3 = cr3;
//     (*tcb)->cpu.esp = I686_KERNEL_STACK_TOP;
//     (*tcb)->cpu.esp0 = I686_KERNEL_STACK_TOP;
    
//     (*tcb)->cpu.ds = GDT_OFFSET(GDT_KERNEL_DS);
//     (*tcb)->cpu.es = (*tcb)->cpu.ds;
//     (*tcb)->cpu.fs = (*tcb)->cpu.ds;
//     (*tcb)->cpu.gs = (*tcb)->cpu.ds;

//     (*tcb)->userStackSize = I686_PROCESS_INITIAL_STACK_SIZE;
//     (*tcb)->kernelStackSize = I686_KERNEL_INITIAL_STACK_SIZE;

//     //all processes start executing by calling a fundamental bootstrap routine
//     //this routine sets up stack and then calls the provided entry point
//     //after the entry point routine returns (the process exits), control is returned
//     //to the bootstrap routine again
//     //the stack layout is as follows (top to bottom):
//     //1. arguments for the bootstrap routine: entry point and entry context
//     //2. architecture specific elements - these are popped by the CPU on IRET:
//     //- EFLAGS - interrupt flag, reserved bits
//     //- CS - privileged mode code segment
//     //- EIP - process entry point
//     //3. return (jump) address on returning from interrupt - this is also popped by the CPU on IRET
//     //4. GP registers (7): eax, ebx, ecx, edx, esi, edi, ebp - these are popped by the context switch code
//     //there is no ESP and SS, because there is no privilege level switch


//     stack[-1] = (uintptr_t)entryContext;
//     stack[-2] = (uintptr_t)entry;
//     stack[-3] = 0; //return address for bootstrap routine - never returns
//     stack[-4] = I686_EFLAGS_IF | I686_EFLAGS_RESERVED;
//     stack[-5] = GDT_OFFSET(GDT_KERNEL_CS);
//     stack[-6] = (uintptr_t)I686ProcessBootstrap;
//     //all 7 GP register, which are zeroed
//     (*tcb)->cpu.esp -= (13 * sizeof(uint32_t)); //update ESP

//     (*tcb)->pid = (*tcb)->tid;

//     MmUnmapDynamicMemory((void*)((uintptr_t)stack - MM_PAGE_SIZE));
//     return OK;

// HalCreateProcessRawExit:
//     if(NULL != (*tcb)->cpu.userPageTableLock)
//         KeDestroySpinlock((*tcb)->cpu.userPageTableLock);
//     KeDestroyTCB(*tcb);
//     *tcb = NULL;

//     return status;
// }

STATUS HalCreateProcessRaw(const char *name, const char *path, PrivilegeLevel pl, 
    void (*entry)(void*), void *entryContext, struct KeTaskControlBlock **tcb)
{
    STATUS status = OK;
    PADDRESS cr3 = 0;
    uint32_t *stack = NULL;
    *tcb = NULL;

    *tcb = KePrepareTCB(pl, name, path);
    if(NULL == *tcb)
    {
        status = OUT_OF_RESOURCES;
        goto HalCreateProcessRawExit;
    }

    (*tcb)->cpu.userPageTableLock = KeCreateSpinlock();
    if(NULL == (*tcb)->cpu.userPageTableLock)
    {
        status = OUT_OF_RESOURCES;
        goto HalCreateProcessRawExit;
    }

    //TODO: randomize stack location
    status = I686CreateProcessMemorySpace(&cr3, I686_KERNEL_STACK_SPACE_START + I686_SINGLE_KERNEL_STACK_SPACE_SIZE, (void*)&stack);
    if(OK != status)
        goto HalCreateProcessRawExit;
    
    CmMemset((void*)((uintptr_t)stack - MM_PAGE_SIZE), 0, MM_PAGE_SIZE);

    (*tcb)->cpu.cr3 = cr3;
    (*tcb)->cpu.esp = I686_KERNEL_STACK_SPACE_START + I686_SINGLE_KERNEL_STACK_SPACE_SIZE;
    (*tcb)->cpu.esp0 = (*tcb)->cpu.esp0;
    
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

HalCreateProcessRawExit:
    if(NULL != (*tcb)->cpu.userPageTableLock)
        KeDestroySpinlock((*tcb)->cpu.userPageTableLock);
    KeDestroyTCB(*tcb);
    *tcb = NULL;

    return status;
}

void HalInitializeScheduler(void)
{
    //create kernel initialization task
    //this is the very first task, so stack pointers are not needed
    //they will be filled at the first task context store event
    struct KeTaskControlBlock *tcb = KePrepareTCB(PL_KERNEL, "KernelInit", NULL);
    if(NULL == tcb)
        KePanicEx(BOOT_FAILURE, KE_SCHEDULER_INITIALIZATION_FAILURE, KE_TCB_PREPARATION_FAILURE, 0, 0);
    
    tcb->cpu.cr3 = I686GetPageDirectoryAddress();

    KeChangeTaskMajorPriority(tcb, TCB_DEFAULT_MAJOR_PRIORITY);
    KeChangeTaskMinorPriority(tcb, TCB_DEFAULT_MINOR_PRIORITY);
    KeEnableTask(tcb);

    extern struct KeTaskControlBlock *currentTask;
    extern void *KeCurrentCpuState;
    currentTask = tcb;
    KeCurrentCpuState = &(tcb->cpu);
}

STATUS HalAttachToTask(struct KeTaskControlBlock *target)
{
    struct KeTaskControlBlock *current = KeGetCurrentTask();

    return OK;
}

static NORETURN void I686ProcessBootstrap(void (*entry)(void*), void *context)
{
    entry(context);
    while(1)
        ;
}

#endif