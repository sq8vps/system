#include "task.h"
#include "mm/heap.h"
#include "rtl/string.h"
#include "hal/hal.h"
#include "hal/math.h"
#include "ke/sched/sched.h"
#include "ex/load.h"
#include "io/fs/fs.h"
#include "hal/arch.h"
#include "hal/task.h"

KE_TASK_ID KeAssignTid(void);
void KeFreeTid(KE_TASK_ID tid);

struct KeProcessControlBlock* KePreparePCB(PrivilegeLevel pl, const char *path, uint32_t flags)
{
    struct KeProcessControlBlock *pcb = MmAllocateKernelHeapZeroed(sizeof(*pcb) + ((NULL != path) ? (RtlStrlen(path) + 1) : 0));
    if(NULL == pcb)
        return NULL;

    ObInitializeObjectHeader(pcb);

    pcb->pl = pl;
    pcb->flags = flags;

    if(NULL != path)
        RtlStrcpy(pcb->path, path);
    
    return pcb;
}

struct KeTaskControlBlock* KePrepareTCB(const char *name, uint32_t flags)
{
    if(NULL == name)
        return NULL;
    
    struct KeTaskControlBlock *tcb = MmAllocateKernelHeapZeroed(sizeof(*tcb) + RtlStrlen(name) + 1);
    if(NULL == tcb)
        return NULL;

    ObInitializeObjectHeader(tcb);

    tcb->flags = flags;
    tcb->affinity = HAL_CPU_ALL;
    tcb->state = TASK_UNINITIALIZED;
    tcb->majorPriority = TCB_DEFAULT_MAJOR_PRIORITY;
    tcb->minorPriority = TCB_DEFAULT_MINOR_PRIORITY;

    tcb->tid = KeAssignTid();
    if(0 == tcb->tid)
    {
        MmFreeKernelHeap(tcb);
        return NULL;
    }

    RtlStrcpy(tcb->name, name);

    return tcb;
}

STATUS KeAssociateTCB(struct KeProcessControlBlock *pcb, struct KeTaskControlBlock *tcb)
{
    PRIO prio = ObLockObject(pcb);
    if(0 == pcb->tasks.count)
    {
        pcb->tasks.list = tcb;
        pcb->tasks.count = 1;
    }
    else if(pcb->tasks.count < UINT32_MAX)
    {
        struct KeTaskControlBlock *t = pcb->tasks.list;
        while(NULL != t->sibling)
            t = t->sibling;
        t->sibling = tcb;
    }
    else
    {
        ObUnlockObject(pcb, prio);
        return KE_KERNEL_THREAD_LIMIT_REACHED;
    }

    tcb->parent = pcb;

    ObUnlockObject(pcb, prio);

    return OK;
}

void KeDissociateTCB(struct KeTaskControlBlock *tcb)
{
    if(NULL == tcb->parent)
        return;

    PRIO prio = ObLockObject(tcb->parent);
    
    struct KeTaskControlBlock *t = tcb->parent->tasks.list;
    if(tcb == t)
    {
        tcb->parent->tasks.list = tcb->sibling;
    }
    else
    {
        while(tcb != t->sibling)
            t = t->sibling;
        
        t->sibling = tcb->sibling;
    }

    tcb->parent->tasks.count--;

    ObUnlockObject(tcb->parent, prio);
    tcb->parent = NULL;
}

void KeDestroyTCB(struct KeTaskControlBlock *tcb)
{
    if(NULL == tcb)
        return;
    KeDissociateTCB(tcb);
    KeFreeTid(tcb->tid);
    MmFreeKernelHeap(tcb);
}

void KeDestroyPCB(struct KeProcessControlBlock *pcb)
{
    MmFreeKernelHeap(pcb);
}

STATUS KeCreateKernelProcess(const char *name, uint32_t flags, void (*entry)(void*), void *entryContext, struct KeTaskControlBlock **tcb)
{  
    return HalCreateProcess(name, NULL, PL_KERNEL, flags, entry, entryContext, tcb);
}

STATUS KeCreateUserProcess(const char *name, const char *path, uint32_t flags, struct KeTaskControlBlock **tcb)
{
    if((NULL == path) || ('\0' == path[0]))
        return IO_FILE_NOT_FOUND;
    return HalCreateProcess(name, path, PL_USER, flags, NULL, NULL, tcb);
}

STATUS KeCreateKernelThread(struct KeProcessControlBlock *pcb, const char *name, uint32_t flags,
    void (*entry)(void*), void *entryContext, struct KeTaskControlBlock **tcb)
{
    return HalCreateThread(pcb, name, flags, entry, entryContext, NULL, tcb);
}

STATUS KeCreateUserThread(const char *name, uint32_t flags,
    void (*entry)(void*), void *entryContext, void *userStack, struct KeTaskControlBlock **tcb)
{
    return HalCreateThread(KeGetCurrentTaskParent(), name, flags, entry, entryContext, userStack, tcb);
}