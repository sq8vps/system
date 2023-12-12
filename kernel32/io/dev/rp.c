#include "rp.h"
#include "mm/heap.h"
#include "assert.h"
#include "common.h"
#include "ke/core/panic.h"

STATUS IoCreateRp(struct IoDriverRp **rp)
{
    *rp = MmAllocateKernelHeap(sizeof(**rp));
    if(NULL == (*rp))
        return OUT_OF_RESOURCES;
    
    CmMemset(*rp, 0, sizeof(**rp));
    return OK;
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

STATUS IoStartRp(struct IoRpQueue *queue, struct IoDriverRp *rp, IoCancelRpCallback cancelCb)
{
    ASSERT(NULL != rp);
    ASSERT(NULL != queue);
    rp->cancelCallback = cancelCb;
    KeAcquireSpinlock(&(queue->queueLock));
    struct IoDriverRp *t = queue->head;
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

STATUS IoFinalizeRp(struct IoDriverRp *rp)
{
    ASSERT(NULL != rp);
    if(NULL != rp->queue)
    {
        if((NULL == rp->queue->head) || (rp->queue->head != rp))
        {
            KePanicIPEx((uintptr_t)__builtin_extract_return_addr(__builtin_return_address(0)), RP_FINALIZED_OUT_OF_LINE, (uintptr_t)rp, 0, 0, 0);
            //no return
        }
        if(rp->flags & IO_DRIVER_RP_FLAG_SYNCHRONOUS)
            rp->completed = true;
        else if(NULL != rp->completionCallback)
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
        if(rp->flags & IO_DRIVER_RP_FLAG_SYNCHRONOUS)
            rp->completed = true;
        else if(NULL != rp->completionCallback)
            rp->completionCallback(rp, rp->completionContext);
        rp->completed = true;
    }
    return OK;
}

STATUS IoCancelRp(struct IoDriverRp *rp)
{
    if(NULL == rp->queue)
        return IO_RP_NOT_CANCELLABLE;

    KeAcquireSpinlock(&(rp->queue->queueLock));
    if(rp->queue->head == rp)
    {
        KeReleaseSpinlock(&(rp->queue->queueLock)); 
        return IO_RP_NOT_CANCELLABLE;
    }
    struct IoDriverRp *t = rp->queue->head;
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