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

#define I686_KERNEL_STACK_TOP (MM_KERNEL_ADDRESS)
#define I686_KERNEL_INITIAL_STACK_SIZE 4096
#define I686_KERNEL_MAX_STACK_SIZE 16384

#define I686_PROCESS_STACK_TOP (I686_KERNEL_STACK_TOP - I686_KERNEL_MAX_STACK_SIZE - MM_PAGE_SIZE)
#define I686_PROCESS_INITIAL_STACK_SIZE 4096
#define I686_PROCESS_MAX_STACK_SIZE 65536

#define I686_EFLAGS_IF (1 << 9) //interrupt flag
#define I686_EFLAGS_RESERVED (1 << 1) //reserved EFLAGS bits
#define I686_EFLAGS_IOPL_USER (3 << 12) //user mode EFLAGS bits

static void CleanupAfterReturn(void);

STATUS HalCreateProcessRaw(const char *name, const char *path, PrivilegeLevel pl, 
    void (*entry)(void*), void *entryContext, struct KeTaskControlBlock **tcb)
{  
    uintptr_t pageDir = CreateProcessPageDirectory();
    if(0 == pageDir)
        return EXEC_PROCESS_PAGE_DIRECTORY_CREATION_FAILURE;

    PRIO prio = HalRaisePriorityLevel(HAL_PRIORITY_LEVEL_EXCLUSIVE);
    
    uintptr_t originalPageDir = GetPageDirectoryAddress();
    SwitchPageDirectory(pageDir); //switch to the new page directory

    STATUS ret = OK;

    if(PL_KERNEL != pl)
    {
        //allocate task stack
        if(OK != (ret = MmAllocateMemory(I686_PROCESS_STACK_TOP - I686_PROCESS_INITIAL_STACK_SIZE, I686_PROCESS_INITIAL_STACK_SIZE,
            MM_FLAG_WRITABLE | MM_FLAG_USER_MODE)))
            goto KeCreateProcessRawExit;

        CmMemset((void*)(I686_PROCESS_STACK_TOP - I686_PROCESS_INITIAL_STACK_SIZE), 0, I686_PROCESS_INITIAL_STACK_SIZE);
    }

    //allocate kernel stack
    if(OK != (ret = MmAllocateMemory(I686_KERNEL_STACK_TOP - I686_KERNEL_INITIAL_STACK_SIZE, I686_KERNEL_INITIAL_STACK_SIZE, MM_FLAG_WRITABLE)))
    {
        goto KeCreateProcessRawExit;
    }

    CmMemset((void*)(I686_KERNEL_STACK_TOP - I686_KERNEL_INITIAL_STACK_SIZE), 0, I686_KERNEL_INITIAL_STACK_SIZE);

    *tcb = KePrepareTCB(pl, name, path);
    if(NULL == *tcb)
    {
        if(PL_KERNEL != pl)
            MmFreeMemory(I686_PROCESS_STACK_TOP - I686_PROCESS_INITIAL_STACK_SIZE, I686_PROCESS_INITIAL_STACK_SIZE);

        MmFreeMemory(I686_KERNEL_STACK_TOP - I686_KERNEL_INITIAL_STACK_SIZE, I686_KERNEL_INITIAL_STACK_SIZE);
        goto KeCreateProcessRawExit;
    }

    (*tcb)->cpu.cr3 = pageDir;
    (*tcb)->cpu.esp = (PL_KERNEL == pl) ? I686_KERNEL_STACK_TOP : I686_PROCESS_STACK_TOP;
    (*tcb)->cpu.esp0 = I686_KERNEL_STACK_TOP;
    
    (*tcb)->cpu.ds = GDT_OFFSET((PL_KERNEL == pl) ? GDT_KERNEL_DS : GDT_USER_DS);
    (*tcb)->cpu.es = (*tcb)->cpu.ds;
    (*tcb)->cpu.fs = (*tcb)->cpu.ds;
    (*tcb)->cpu.gs = (*tcb)->cpu.ds;

    (*tcb)->userStackSize = I686_PROCESS_INITIAL_STACK_SIZE;
    (*tcb)->kernelStackSize = I686_KERNEL_INITIAL_STACK_SIZE;

    uint32_t *stack = (uint32_t*)((*tcb)->cpu.esp); //get kernel stack
    //start task in kernel mode initially and allow it to load the process image on its own
    //this requires an appropriate stack layout to perform iret on task switch
    //the stack layout is as follows (top to bottom):
    //return address after the process exits
    //argument for process entry pointer
    //EFLAGS - interrupt flag, reserved bits
    //CS - privileged mode code segment
    //EIP - process entry point
    //GP registers (6): eax, ebx, ecx, edx, esi, edi
    //ebp register - equal to esp
    //there is no ESP and SS, because there is no privilege level switch

    //GP registers are zeroed
    stack[-1] = (uintptr_t)entryContext;
    stack[-2] = (uintptr_t)CleanupAfterReturn;
    stack[-3] = I686_EFLAGS_IF | I686_EFLAGS_RESERVED;
    stack[-4] = GDT_OFFSET(GDT_KERNEL_CS);
    stack[-5] = (uintptr_t)entry;
    stack[-12] = (*tcb)->cpu.esp; //set EBP
    (*tcb)->cpu.esp -= (12 * sizeof(uint32_t)); //update ESP

    (*tcb)->pid = (*tcb)->tid;

KeCreateProcessRawExit:
    SwitchPageDirectory(originalPageDir);
    HalLowerPriorityLevel(prio);

    return ret;
}

void HalInitializeScheduler(void)
{
    //create kernel initialization task
    uintptr_t cr3;
    ASM("mov %0,cr3" : "=r" (cr3) : );
    //this is the very first task, so stack pointers are not needed
    //there will be filled at the first task context store event
    struct KeTaskControlBlock *tcb = KePrepareTCB(PL_KERNEL, "KernelInit", NULL);
    if(NULL == tcb)
        KePanicEx(BOOT_FAILURE, KE_SCHEDULER_INITIALIZATION_FAILURE, KE_TCB_PREPARATION_FAILURE, 0, 0);
    
    tcb->cpu.cr3 = cr3;

    KeChangeTaskMajorPriority(tcb, TCB_DEFAULT_MAJOR_PRIORITY);
    KeChangeTaskMinorPriority(tcb, TCB_DEFAULT_MINOR_PRIORITY);
    KeEnableTask(tcb);

    extern struct KeTaskControlBlock *currentTask;
    extern void *KeCurrentCpuState;
    currentTask = tcb;
    KeCurrentCpuState = &(tcb->cpu);
}

static void CleanupAfterReturn(void)
{
    while(1)
        ;
}

#endif