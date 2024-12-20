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
    struct KeProcessControlBlock *pcb = ObCreateKernelObjectEx(OB_PCB, ((NULL != path) ? (RtlStrlen(path) + 1) : 0));
    if(NULL == pcb)
        return NULL;

    pcb->pl = pl;
    pcb->flags = flags;

    if(NULL != path)
        RtlStrcpy(pcb->path, path);
    
    return pcb;
}

struct KeTaskControlBlock* KePrepareTCB(uint32_t flags)
{
    struct KeTaskControlBlock *tcb = ObCreateObject(OB_TCB);
    if(NULL == tcb)
        return NULL;

    tcb->flags = flags;
    tcb->affinity = HAL_CPU_ALL;
    tcb->scheduling.state = TASK_UNINITIALIZED;
    tcb->scheduling.majorPriority = TCB_DEFAULT_MAJOR_PRIORITY;
    tcb->scheduling.minorPriority = TCB_DEFAULT_MINOR_PRIORITY;

    tcb->tid = KeAssignTid();
    if(0 == tcb->tid)
    {
        ObDestroyObject(tcb);
        return NULL;
    }

    return tcb;
}

STATUS KeAssociateTCB(struct KeProcessControlBlock *pcb, struct KeTaskControlBlock *tcb)
{
    PRIO prio = KeAcquireSpinlock(&(pcb->tasks.lock));
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
        KeReleaseSpinlock(&(pcb->tasks.lock), prio);
        return KERNEL_THREAD_LIMIT_REACHED;
    }

    tcb->parent = pcb;
    ObChangeObjectOwner(tcb, pcb);

    KeReleaseSpinlock(&(pcb->tasks.lock), prio);

    return OK;
}

void KeDissociateTCB(struct KeTaskControlBlock *tcb)
{
    if(NULL == tcb->parent)
        return;

    PRIO prio = KeAcquireSpinlock(&(tcb->parent->tasks.lock));
    
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

    KeReleaseSpinlock(&(tcb->parent->tasks.lock), prio);
    tcb->parent = NULL;
}

void KeDestroyTCB(struct KeTaskControlBlock *tcb)
{
    if(NULL == tcb)
        return;
    KeDissociateTCB(tcb);
    KeFreeTid(tcb->tid);
    ObDestroyObject(tcb);
}

void KeDestroyPCB(struct KeProcessControlBlock *pcb)
{
    ObDestroyObject(pcb);
}

static STATUS KeCloneFileHandles(struct KeProcessControlBlock *pcb, const struct KeTaskFileMapping *map)
{
    STATUS status = OK;
    if(NULL == map)
        return OK;
    
    const struct KeTaskFileMapping *m = map;
    while((m->mapFrom >= 0) && (m->mapTo >= 0))
    {
        status = IoCloneFileToNewProcess(pcb, m->mapTo, m->mapFrom);
        if(OK != status)
        {
            const struct KeTaskFileMapping *last = m;
            m = map;
            while(m < last)
            {
                IoCloseFileForProcess(pcb, m->mapTo);
                ++m;
            }
            return status;
        }
        ++m;
    }
    return OK;
}

STATUS KeCreateKernelProcess(uint32_t flags, void (*entry)(void*), void *entryContext, struct KeTaskFileMapping *fileMap, struct KeTaskControlBlock **tcb)
{  
    STATUS status = OK;
    
    status = HalCreateProcess(NULL, PL_KERNEL, flags, entry, entryContext, tcb);
    if(OK != status)
        return status;
    
    status = KeCloneFileHandles((*tcb)->parent, fileMap);
    if(OK != status)
    {
        //TODO: destroy TCB and PCB
        return status;
    }
    return OK;
}

STATUS KeCreateUserProcess(const char *path, uint32_t flags, const char *argv[], const char *envp[], struct KeTaskFileMapping *fileMap, struct KeTaskControlBlock **tcb)
{
    STATUS status = OK;
    if((NULL == path) || ('\0' == path[0]))
        return FILE_NOT_FOUND;
    struct KeTaskArguments *args = KeBuildTaskArguments(argv, envp);
    if(NULL == args)
        return OUT_OF_RESOURCES;

    status = HalCreateProcess(path, PL_USER, flags, NULL, args, tcb);
    if(OK != status)
        return status;
    
    status = KeCloneFileHandles((*tcb)->parent, fileMap);
    if(OK != status)
    {
        //TODO: destroy TCB and PCB
        return status;
    }
    return OK;
}

STATUS KeCreateKernelThread(struct KeProcessControlBlock *pcb, uint32_t flags,
    void (*entry)(void*), void *entryContext, struct KeTaskControlBlock **tcb)
{
    return HalCreateThread(pcb, flags, entry, entryContext, NULL, tcb);
}

STATUS KeCreateUserThread(uint32_t flags,
    void (*entry)(void*), void *entryContext, void *userStack, struct KeTaskControlBlock **tcb)
{
    return HalCreateThread(KeGetCurrentTaskParent(), flags, entry, entryContext, userStack, tcb);
}

struct KeTaskArguments* KeBuildTaskArguments(const char *argv[], const char *envp[])
{
    int argc = 0, envc = 0;
    size_t length = 0;
    if(NULL != argv)
    {
        while(NULL != argv[argc])
        {
            length += RtlStrlen(argv[argc]) + 1;
            ++argc;
        }
    }
    if(NULL != envp)
    {
        while(NULL != envp[envc])
        {
            length += RtlStrlen(envp[envc]) + 1;
            ++envc;
        }
    }

    struct KeTaskArguments *s = MmAllocateKernelHeap(sizeof(*s) + length);
    if(NULL == s)
        return NULL;

    s->argc = argc;
    s->envc = envc;
    s->size = length;

    size_t index = 0;
    for(int i = 0; i < argc; i++)
    {
        size_t k = 0;
        do
            s->data[index++] = argv[i][k];
        while('\0' != argv[i][k++]);
    }
    for(int i = 0; i < envc; i++)
    {
        size_t k = 0;
        do
            s->data[index++] = envp[i][k];
        while('\0' != envp[i][k++]);
    }
    return s;
}
