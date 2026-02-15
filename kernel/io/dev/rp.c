#include "rp.h"
#include "mm/heap.h"
#include "assert.h"
#include "rtl/string.h"
#include "ke/core/panic.h"
#include "ke/sched/sched.h"
#include "mm/slab.h"
#include "io/fs/vfs.h"
#include "res.h"
#include "ke/task/task.h"
#include "dev.h"

struct IoRp *IoCreateRp(void)
{
    return ObCreateKernelObject(OB_RP);
}

void IoFreeRp(struct IoRp *rp)
{
    ObDestroyObject(rp);
}

STATUS IoCreateRpQueue(IoProcessRpCallback callback, struct IoRpQueue **queue)
{
    ASSERT(NULL != queue);

    if(NULL == callback)
        return BAD_PARAMETER;

    *queue = MmAllocateKernelHeap(sizeof(**queue));
    if(NULL == *queue)
        return OUT_OF_RESOURCES;
    
    RtlMemset(*queue, 0, sizeof(**queue));
    (*queue)->callback = callback;
    (*queue)->queueLock = (KeSpinlock)KeSpinlockInitializer;
    return OK;
}

STATUS IoDestroyRpQueue(struct IoRpQueue *queue)
{
    MmFreeKernelHeap(queue);
    return OK;
}

STATUS IoStartRp(struct IoRpQueue *queue, struct IoRp *rp, IoRpCancelCallback cancelCb)
{
    ASSERT(NULL != rp);
    ASSERT(NULL != queue);
    rp->cancelCallback = cancelCb;
    PRIO prio = KeAcquireSpinlock(&(queue->queueLock));
    struct IoRp *t = queue->head;
    if(NULL == t)
    {
        queue->head = rp;
        t = rp;
    }
    else
    {
        while(NULL != t->next)
        {
            ASSERT(t != rp);
            t = t->next;
        }
        ASSERT(t != rp);
        t->next = rp;
    }
    rp->next = NULL;
    rp->queue = queue;

    if(!queue->busy)
    {
        queue->busy = 1;
        KeReleaseSpinlock(&(queue->queueLock), prio);
        queue->callback(t);
        return OK;
    }
    else
    {
        KeReleaseSpinlock(&(queue->queueLock), prio);
        return OK;
    }
}

STATUS IoFinalizeRp(struct IoRp *rp)
{
    ASSERT(NULL != rp);
    if(NULL != rp->queue)
    {
        if(unlikely((NULL == rp->queue->head) || (rp->queue->head != rp)))
        {
            KePanicIPEx(GET_CALLER_ADDRESS(0), RP_FINALIZED_OUT_OF_ORDER, (uintptr_t)rp, (uintptr_t)rp->queue, 0, 0);
            //no return
        }
        if(NULL != rp->completionCallback)
            rp->completionCallback(rp, rp->completionContext);

        PRIO prio = KeAcquireSpinlock(&(rp->queue->queueLock));
        ASSERT(rp != rp->next);
        rp->queue->head = rp->next;
        if(NULL == rp->queue->head)
        {
            rp->queue->busy = 0;
            KeReleaseSpinlock(&(rp->queue->queueLock), prio);
        }
        else
        {
            KeReleaseSpinlock(&(rp->queue->queueLock), prio);
            rp->queue->callback(rp->queue->head);
        }
        rp->queue = NULL;
    }
    else
    {
        if(NULL != rp->completionCallback)
            rp->completionCallback(rp, rp->completionContext);
    }

    PRIO lastPrio = HalRaisePriorityLevel(HAL_PRIORITY_LEVEL_EXCLUSIVE);
    barrier();
    if(rp->sync && rp->pending)
        KeUnblockTask(rp->task);
    rp->pending = false;
    barrier();
    HalLowerPriorityLevel(lastPrio);

    if(NULL != rp->completionCallback)
        IoFreeRp(rp);

    return OK;
}

STATUS IoCancelRp(struct IoRp *rp)
{
    if(NULL == rp->queue)
        return BAD_PARAMETER;

    PRIO prio = KeAcquireSpinlock(&(rp->queue->queueLock));
    if(rp->queue->head == rp)
    {
        KeReleaseSpinlock(&(rp->queue->queueLock), prio); 
        return BUSY;
    }
    struct IoRp *t = rp->queue->head;
    while(NULL != t)
    {
        if(t == rp)
        {
            if(NULL != t->next)
                t->next->previous = t->previous;
            if(NULL != t->previous)
                t->previous->next = t->next;
            
            KeReleaseSpinlock(&(rp->queue->queueLock), prio);
            rp->cancelCallback(rp);
            return OK;
        }
        t = t->next;
    }
    KeReleaseSpinlock(&(rp->queue->queueLock), prio); 
    return NOT_FOUND;
}

struct IoDeviceObject* IoGetCurrentRpPosition(struct IoRp *rp)
{
    ASSERT(rp);
    return rp->device;
}

void IoMarkRpPending(struct IoRp *rp)
{
    ASSERT(rp);
    rp->pending = true;
}

struct IoRp *IoCloneRp(struct IoRp *rp)
{
    if(NULL == rp)
        return rp;

    struct IoRp *n = ObCreateKernelObject(OB_RP);
    if(NULL == n)
        return NULL;
    
    RtlMemcpy(n, rp, sizeof(*rp));
    n->queue = NULL;
    n->next = NULL;
    n->previous = NULL;
    return n;
}