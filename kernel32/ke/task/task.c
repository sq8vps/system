#include "task.h"
#include "mm/heap.h"
#include "it/it.h"
#include "common.h"
#include "hal/hal.h"
#include "mm/gdt.h"
#include "mm/valloc.h"
#include "mm/mm.h"
#include "ke/sched/sched.h"
#include "ke/core/mutex.h"
#include "ex/load.h"

#define KE_KERNEL_STACK_TOP (MM_KERNEL_ADDRESS)
#define KE_KERNEL_INITIAL_STACK_SIZE 4096
#define KE_KERNEL_MAX_STACK_SIZE 16384

#define KE_PROCESS_STACK_TOP (KE_KERNEL_STACK_TOP - KE_KERNEL_MAX_STACK_SIZE - MM_PAGE_SIZE)
#define KE_PROCESS_INITIAL_STACK_SIZE 4096
#define KE_PROCESS_MAX_STACK_SIZE 65536

struct KeTaskControlBlock* KePrepareTCB(uintptr_t kernelStack, uintptr_t stack, uintptr_t pageDir, PrivilegeLevel_t pl, const char *name, const char *path)
{
    struct KeTaskControlBlock *tcb = MmAllocateKernelHeap(sizeof(struct KeTaskControlBlock));
    if(NULL == tcb)
        return NULL;
    
    CmMemset((void*)tcb, 0, sizeof(*tcb));

    ObInitializeObjectHeader(tcb);

    tcb->mathState = HalCreateMathStateBuffer();
    if(NULL == tcb->mathState)
    {
        MmFreeKernelHeap(tcb);
        return NULL;
    }

    tcb->cpu.cr3 = pageDir;
    tcb->cpu.esp = stack;
    tcb->cpu.esp0 = kernelStack;
    
    if(PL_KERNEL == pl)
        tcb->cpu.ds = MmGdtGetFlatPrivilegedDataOffset();
    else
        tcb->cpu.ds = MmGdtGetFlatUnprivilegedDataOffset();

    tcb->cpu.es = tcb->cpu.ds;
    tcb->cpu.fs = tcb->cpu.ds;
    tcb->cpu.gs = tcb->cpu.ds;

    tcb->pl = pl;


    tcb->state = TASK_UNINITIALIZED;
    tcb->state = TASK_UNINITIALIZED;
    tcb->majorPriority = TCB_DEFAULT_MAJOR_PRIORITY;
    tcb->minorPriority = TCB_DEFAULT_MINOR_PRIORITY;

    CmStrncpy(tcb->name, name, TCB_MAX_NAME_LENGTH);
    if((NULL != path) && (CmStrlen(path) > 1))
    {
        if(NULL == (tcb->path = MmAllocateKernelHeap(CmStrlen(path) + 1)))
        {
            MmFreeKernelHeap(tcb);
            return NULL;
        }

        CmStrcpy(tcb->path, path);
    }

    tcb->tid = KeAssignTid();
    tcb->pid = tcb->tid;

    return tcb;
}

NORETURN static void KeCleanupAfterReturn(void)
{
    PRINT("Process %s returned\n", KeGetCurrentTask()->name);
    while(1)
        ;
}

static KeSpinlock processCreationMutex = KeSpinlockInitializer;

STATUS KeCreateProcessRaw(const char *name, const char *path, PrivilegeLevel_t pl, 
    void (*entry)(void*), void *entryContext, struct KeTaskControlBlock **tcb)
{  
    uintptr_t pageDir = MmCreateProcessPageDirectory();
    if(0 == pageDir)
        return EXEC_PROCESS_PAGE_DIRECTORY_CREATION_FAILURE;

    KeAcquireSpinlock(&processCreationMutex); //spinlock to ensure the page directory does not change
    
    uintptr_t originalPageDir = MmGetPageDirectoryAddress();
    MmSwitchPageDirectory(pageDir); //switch to the new page directory

    STATUS ret = OK;

    if(PL_KERNEL != pl)
    {
        //allocate task stack
        if(OK != (ret = MmAllocateMemory(KE_PROCESS_STACK_TOP - KE_PROCESS_INITIAL_STACK_SIZE, KE_PROCESS_INITIAL_STACK_SIZE,
            MM_PAGE_FLAG_WRITABLE | MM_PAGE_FLAG_USER)))
            goto KeCreateProcessRawExit;

        CmMemset((void*)(KE_PROCESS_STACK_TOP - KE_PROCESS_INITIAL_STACK_SIZE), 0, KE_PROCESS_INITIAL_STACK_SIZE);
    }

    //allocate kernel stack
    if(OK != (ret = MmAllocateMemory(KE_KERNEL_STACK_TOP - KE_KERNEL_INITIAL_STACK_SIZE, KE_KERNEL_INITIAL_STACK_SIZE, MM_PAGE_FLAG_WRITABLE)))
    {
        goto KeCreateProcessRawExit;
    }

    CmMemset((void*)(KE_KERNEL_STACK_TOP - KE_KERNEL_INITIAL_STACK_SIZE), 0, KE_KERNEL_INITIAL_STACK_SIZE);

    *tcb = KePrepareTCB(KE_KERNEL_STACK_TOP, (PL_KERNEL == pl) ? KE_KERNEL_STACK_TOP : KE_PROCESS_STACK_TOP, pageDir, pl, name, path);
    if(NULL == *tcb)
    {
        if(PL_KERNEL != pl)
            MmFreeMemory(KE_PROCESS_STACK_TOP - KE_PROCESS_INITIAL_STACK_SIZE, KE_PROCESS_INITIAL_STACK_SIZE);

        MmFreeMemory(KE_KERNEL_STACK_TOP - KE_KERNEL_INITIAL_STACK_SIZE, KE_KERNEL_INITIAL_STACK_SIZE);
        goto KeCreateProcessRawExit;
    }

    (*tcb)->stackSize = KE_PROCESS_INITIAL_STACK_SIZE;

    uint32_t *stack = (uint32_t*)((*tcb)->cpu.esp); //get kernel stack
    //start task in kernel mode initially and allow it to load the process image on its own
    //this requires an appropriate stack layout to perform iret on task switch
    //the stack layout is as follows (top to bottom):
    //argument for process entry pointer
    //return address after the process exits
    //EFLAGS - interrupt flag, reserved bits
    //CS - privileged mode code segment
    //EIP - process entry point
    //GP registers (6): eax, ebx, ecx, edx, esi, edi
    //ebp register - equal to esp
    //there is no ESP and SS, because there is no privilege level switch

    //GP registers are zeroed
    stack[-1] = (uintptr_t)entryContext;
    stack[-2] = (uintptr_t)KeCleanupAfterReturn;
    stack[-3] = TCB_EFLAGS_IF | TCB_EFLAGS_RESERVED;
    stack[-4] = MmGdtGetFlatPrivilegedCodeOffset();
    stack[-5] = (uintptr_t)entry;
    stack[-12] = (*tcb)->cpu.esp; //set EBP
    (*tcb)->cpu.esp -= (12 * sizeof(uint32_t)); //update ESP

    (*tcb)->pid = (*tcb)->tid;

KeCreateProcessRawExit:
    MmSwitchPageDirectory(originalPageDir);
    KeReleaseSpinlock(&processCreationMutex);

    return ret;
}

STATUS KeCreateProcess(const char *name, const char *path, PrivilegeLevel_t pl, struct KeTaskControlBlock **tcb)
{
    return KeCreateProcessRaw(name, path, pl, (void(*)(void*))ExProcessLoadWorker, (*tcb)->path, tcb);
}

uintptr_t KeGetHighestAvailableMemory(void)
{
    return KE_PROCESS_STACK_TOP - KE_PROCESS_MAX_STACK_SIZE - MM_PAGE_SIZE - 1;
}