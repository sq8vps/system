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
    struct KeTaskControlBlock *tcb = MmAllocateKernelHeap(sizeof(struct KeTaskControlBlock));
    if(NULL == tcb)
        return NULL;
    
    CmMemset((void*)tcb, 0, sizeof(*tcb));

    ObInitializeObjectHeader(tcb);

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

STATUS KeCreateProcessRaw(const char *name, const char *path, PrivilegeLevel pl, 
    void (*entry)(void*), void *entryContext, struct KeTaskControlBlock **tcb)
{  
    return HalCreateProcessRaw(name, path, pl, entry, entryContext, tcb);
}

STATUS KeCreateProcess(const char *name, const char *path, PrivilegeLevel pl, struct KeTaskControlBlock **tcb)
{
    return KeCreateProcessRaw(name, path, pl, (void(*)(void*))ExProcessLoadWorker, (*tcb)->path, tcb);
}