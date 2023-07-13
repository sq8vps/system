#include "task.h"
#include "../mm/heap.h"
#include "../it/it.h"
#include "../common.h"
#include "../hal/hal.h"
#include "../mm/gdt.h"
#include "../mm/valloc.h"
#include "../mm/mm.h"
#include "sched.h"
#include "mutex.h"

#define KE_KERNEL_STACK_TOP (MM_KERNEL_ADDRESS)
#define KE_KERNEL_INITIAL_STACK_SIZE 4096
#define KE_KERNEL_STACK_MAX_SIZE 16384

#define KE_PROCESS_STACK_TOP (KE_KERNEL_STACK_TOP - KE_KERNEL_STACK_MAX_SIZE - MM_PAGE_SIZE)
#define KE_PROCESS_INITIAL_STACK_SIZE 4096

struct TaskControlBlock* KePrepareTCB(uintptr_t kernelStack, uintptr_t stack, uintptr_t pageDir, PrivilegeLevel_t pl, const char *name, const char *path)
{
    struct TaskControlBlock *tcb = MmAllocateKernelHeap(sizeof(struct TaskControlBlock));
    if(NULL == tcb)
        return NULL;
    
    Cm_memset((void*)tcb, 0, sizeof(*tcb)); //clear block

    tcb->cr3 = pageDir;
    tcb->esp = stack;
    tcb->esp0 = kernelStack;
    
    if(PL_KERNEL == pl)
        tcb->ds = MmGdtGetFlatPrivilegedDataOffset();
    else
        tcb->ds = MmGdtGetFlatUnprivilegedDataOffset();

    tcb->es = tcb->ds;
    tcb->fs = tcb->ds;
    tcb->gs = tcb->ds;


    tcb->state = TASK_UNINITIALIZED;
    tcb->majorPriority = TCB_DEFAULT_MAJOR_PRIORITY;
    tcb->minorPriority = TCB_DEFAULT_MINOR_PRIORITY;

    tcb->next = NULL;
    tcb->previous = NULL;

    CmStrncpy(tcb->name, name, TCB_MAX_NAME_LENGTH);
    if((NULL != path) && (Cm_strlen(path) > 1))
    {
        if(NULL == (tcb->path = MmAllocateKernelHeap(Cm_strlen(path) + 1)))
        {
            MmFreeKernelHeap(tcb);
            return NULL;
        }
        

        Cm_strcpy(tcb->path, path);
    }

    return tcb;
}


NORETURN static void KeProcessBootstrap(void)
{
    while(1);;
}

static KeSpinLock_t processCreationMutex;

STATUS KeCreateProcessRaw(const char *name, const char *path, PrivilegeLevel_t pl, void (*entry)(), struct TaskControlBlock **tcb)
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
            return ret;

        Cm_memset((void*)(KE_PROCESS_STACK_TOP - KE_PROCESS_INITIAL_STACK_SIZE), 0, KE_PROCESS_INITIAL_STACK_SIZE);
    }

    //allocate kernel stack
    if(OK != (ret = MmAllocateMemory(KE_KERNEL_STACK_TOP - KE_KERNEL_INITIAL_STACK_SIZE, KE_KERNEL_INITIAL_STACK_SIZE, MM_PAGE_FLAG_WRITABLE)))
    {
        return ret;
    }

    Cm_memset((void*)(KE_KERNEL_STACK_TOP - KE_KERNEL_INITIAL_STACK_SIZE), 0, KE_KERNEL_INITIAL_STACK_SIZE);

    *tcb = KePrepareTCB(KE_KERNEL_STACK_TOP, (PL_KERNEL == pl) ? KE_KERNEL_STACK_TOP : KE_PROCESS_STACK_TOP, pageDir, pl, name, path);
    if(NULL == *tcb)
    {
        if(PL_KERNEL != pl)
            MmFreeMemory(KE_PROCESS_STACK_TOP - KE_PROCESS_INITIAL_STACK_SIZE, KE_PROCESS_INITIAL_STACK_SIZE);

        MmFreeMemory(KE_KERNEL_STACK_TOP - KE_KERNEL_INITIAL_STACK_SIZE, KE_KERNEL_INITIAL_STACK_SIZE);
        return KE_TCB_PREPARATION_FAILURE;
    }


    uint32_t *stack = (uint32_t*)((*tcb)->esp); //get kernel stack
    //start task in kernel mode initially and allow it to load the process image on its own
    //this requires an appropriate stack layout to perform iret on task switch
    //the stack layout is as follows (top to bottom):
    //EFLAGS - interrupt flag, reserved bits
    //CS - privileged mode code segment
    //EIP - process entry point
    //GP registers (6): eax, ebx, ecx, edx, esi, edi
    //ebp register - equal to esp
    //there is no ESP and SS, because there is no privilege level switch

    //GP registers are zeroed
    stack[-1] = TCB_EFLAGS_IF | TCB_EFLAGS_RESERVED;
    stack[-2] = MmGdtGetFlatPrivilegedCodeOffset();
    stack[-3] = (uintptr_t)entry;
    stack[-10] = (*tcb)->esp; //set EBP
    (*tcb)->esp -= (10 * sizeof(uint32_t)); //update ESP

    MmSwitchPageDirectory(originalPageDir);
    KeReleaseSpinlock(&processCreationMutex);

    return OK;
}

STATUS KeCreateProcess(const char *name, const char *path, PrivilegeLevel_t pl, struct TaskControlBlock **tcb)
{
    return KeCreateProcessRaw(name, path, pl, KeProcessBootstrap, tcb);
}