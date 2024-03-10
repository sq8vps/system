#include "rp.h"
#include "mm/heap.h"
#include "assert.h"
#include "common.h"
#include "ke/core/panic.h"
#include "ke/sched/sched.h"
#include "mm/slab.h"

#define IO_RP_CACHE_CHUNK_PER_SLAB 64
static void *IoRpSlabCache = NULL;

STATUS IoInitializeRpCache(void)
{
    IoRpSlabCache = MmSlabCreate(sizeof(struct IoRp), IO_RP_CACHE_CHUNK_PER_SLAB);
    if(NULL == IoRpSlabCache)
        return OUT_OF_RESOURCES;
    
    return OK;
}

struct IoRp *IoCreateRp(void)
{
    struct IoRp *rp = MmSlabAllocate(IoRpSlabCache);
    if(NULL == rp)
        return NULL;
    
    CmMemset(rp, 0, sizeof(*rp));
    return rp;
}

void IoFreeRp(struct IoRp *rp)
{
    MmSlabFree(IoRpSlabCache, rp);
}

STATUS IoCreateRpQueue(IoProcessRpCallback callback, struct IoRpQueue **queue)
{
    ASSERT(NULL != queue);

    if(NULL == callback)
        return NULL_POINTER_GIVEN;

    *queue = MmAllocateKernelHeap(sizeof(**queue));
    if(NULL == *queue)
        return OUT_OF_RESOURCES;
    
    CmMemset(*queue, 0, sizeof(**queue));
    (*queue)->callback = callback;
    (*queue)->queueLock.lock = 0;
    return OK;
}

STATUS IoStartRp(struct IoRpQueue *queue, struct IoRp *rp, IoCancelRpCallback cancelCb)
{
    ASSERT(NULL != rp);
    ASSERT(NULL != queue);
    rp->cancelCallback = cancelCb;
    KeAcquireSpinlock(&(queue->queueLock));
    struct IoRp *t = queue->head;
    if(NULL == t)
    {
        queue->head = rp;
        t = rp;
    }
    else
    {
        while(NULL != t->next)
            t = t->next;
        
        t->next = rp;
    }

    rp->queue = queue;

    if(!queue->busy)
    {
        queue->busy = 1;
        KeReleaseSpinlock(&(queue->queueLock));
        queue->callback(t);
        return OK;
    }
    else
    {
        KeReleaseSpinlock(&(queue->queueLock));
        return OK;
    }
}

STATUS IoFinalizeRp(struct IoRp *rp)
{
    ASSERT(NULL != rp);
    if(NULL != rp->queue)
    {
        if((NULL == rp->queue->head) || (rp->queue->head != rp))
        {
            KePanicIPEx(KE_CALLER_ADDRESS(), RP_FINALIZED_OUT_OF_LINE, (uintptr_t)rp, 0, 0, 0);
            //no return
        }
        if(NULL != rp->completionCallback)
            rp->completionCallback(rp, rp->completionContext);

        KeAcquireSpinlock(&(rp->queue->queueLock));
        rp->queue->head = rp->next;
        if(NULL == rp->queue->head)
        {
            rp->queue->busy = 0;
            KeReleaseSpinlock(&(rp->queue->queueLock));
        }
        else
        {
            KeReleaseSpinlock(&(rp->queue->queueLock));
            rp->queue->callback(rp->queue->head);
        }
    }
    else
    {
        if(NULL != rp->completionCallback)
            rp->completionCallback(rp, rp->completionContext);
    }

    uint8_t lastPrio = HalRaiseTaskPriority(HAL_TASK_PRIORITY_EXCLUSIVE);
    if((NULL != rp->task) && rp->pending)
        KeUnblockTask(rp->task);
    rp->pending = false;
    HalLowerTaskPriority(lastPrio);

    return OK;
}

STATUS IoCancelRp(struct IoRp *rp)
{
    if(NULL == rp->queue)
        return IO_RP_NOT_CANCELLABLE;

    KeAcquireSpinlock(&(rp->queue->queueLock));
    if(rp->queue->head == rp)
    {
        KeReleaseSpinlock(&(rp->queue->queueLock)); 
        return IO_RP_NOT_CANCELLABLE;
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
            
            KeReleaseSpinlock(&(rp->queue->queueLock));
            rp->cancelCallback(rp);
            return OK;
        }
        t = t->next;
    }
    KeReleaseSpinlock(&(rp->queue->queueLock)); 
    return IO_RP_NOT_CANCELLABLE;
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

void IoWaitForRpCompletion(struct IoRp *rp)
{
    while(1)
    {
        uint8_t lastPrio = HalRaiseTaskPriority(HAL_TASK_PRIORITY_EXCLUSIVE);
        if(rp->pending)
        {
            KeBlockTask(rp->task, TASK_BLOCK_IO);
            HalLowerTaskPriority(lastPrio);
            KeTaskYield();
        }
        else
        {
            HalLowerTaskPriority(lastPrio);
            break;
        }
    }
}