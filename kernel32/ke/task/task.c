#include "task.h"
#include "mm/heap.h"
#include "common.h"
#include "hal/hal.h"
#include "hal/math.h"
#include "ke/sched/sched.h"
#include "ex/load.h"
#include "io/fs/fs.h"
#include "hal/arch.h"

KE_TASK_ID KeAssignTid(void);
void KeFreeTid(KE_TASK_ID tid);

void KeDestroyTCB(struct KeTaskControlBlock *tcb)
{
    if(NULL == tcb)
        return;
    //KeDestroyMutex(tcb->attach.mutex);
    MmFreeKernelHeap(tcb->mathState);
    MmFreeKernelHeap(tcb->path);
    MmFreeKernelHeap(tcb);
}

struct KeTaskControlBlock* KePrepareTCB(PrivilegeLevel pl, const char *name, const char *path)
{
    struct KeTaskControlBlock *tcb = MmAllocateKernelHeapZeroed(sizeof(struct KeTaskControlBlock));
    if(NULL == tcb)
        return NULL;

    ObInitializeObjectHeader(tcb);

    tcb->affinity = HAL_CPU_ALL;
    tcb->mathState = HalCreateMathStateBuffer();
    if(NULL == tcb->mathState)
    {
        KeDestroyTCB(tcb);
        return NULL;
    }

    for(uintptr_t i = 0; i < MAX_KERNEL_MODE_THREADS - 1; i++)
    {
        tcb->freeTaskIds[i] = MAX_KERNEL_MODE_THREADS - 1 - i;
    }

    tcb->pl = pl;
    // tcb->attach.mutex = KeCreateMutex();
    // if(NULL == tcb->attach.mutex)
    // {
    //     KeDestroyTCB(tcb);
    //     return NULL;
    // }

    tcb->state = TASK_UNINITIALIZED;
    tcb->state = TASK_UNINITIALIZED;
    tcb->majorPriority = TCB_DEFAULT_MAJOR_PRIORITY;
    tcb->minorPriority = TCB_DEFAULT_MINOR_PRIORITY;

    CmStrncpy(tcb->name, name, TCB_MAX_NAME_LENGTH);
    if((NULL != path) && (CmStrlen(path) > 1))
    {
        if(NULL == (tcb->path = MmAllocateKernelHeap(CmStrlen(path) + 1)))
        {
            KeDestroyTCB(tcb);
            return NULL;
        }

        CmStrcpy(tcb->path, path);
    }

    tcb->tid = KeAssignTid();

    return tcb;
}

STATUS KeCreateKernelProcess(const char *name, void (*entry)(void*), void *entryContext, struct KeTaskControlBlock **tcb)
{  
    return HalCreateProcess(name, NULL, PL_KERNEL, entry, entryContext, tcb);
}

STATUS KeCreateUserProcess(const char *name, const char *path, struct KeTaskControlBlock **tcb)
{
    if((NULL == path) || ('\0' == path[0]))
        return IO_FILE_NOT_FOUND;
    return HalCreateProcess(name, path, PL_USER, NULL, NULL, tcb);
}

STATUS KeCreateThread(struct KeTaskControlBlock *parent, const char *name,
    void (*entry)(void*), void *entryContext, struct KeTaskControlBlock **tcb)
{
    return HalCreateThread(parent, name, entry, entryContext, tcb);
}