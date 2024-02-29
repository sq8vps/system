#include "enumeration.h"
#include "ke/task/task.h"
#include "ke/sched/sched.h"
#include "io/dev/dev.h"
#include "ke/core/mutex.h"
#include "mm/heap.h"



static struct KeTaskControlBlock *enumeratorThread;
struct IoEnumerationQueue
{
    struct IoDeviceNode *node;
    struct IoEnumerationQueue *next;
};
static struct IoEnumerationQueue *IoEnumerationQueueHead = NULL;
static KeSpinlock IoEnumerationQueueMutex = KeSpinlockInitializer;

#include "mm/palloc.h"
#include "mm/mmio.h"
#include "io/dev/types.h"
#include "common.h"
STATUS cb(struct IoRp *rp, void *context)
{
    PRINT("JEST %X!!\n", ((uint8_t*)(((struct IoMemoryDescriptor*)context)->mapped))[511]);
    return OK;
}

static void IoEnumeratorThread(void *unused)
{
    while(1)
    {
        KeAcquireSpinlock(&IoEnumerationQueueMutex);
        while(NULL != IoEnumerationQueueHead)
        {
            struct IoEnumerationQueue *t = IoEnumerationQueueHead;
            IoEnumerationQueueHead = t->next;
            KeReleaseSpinlock(&IoEnumerationQueueMutex);
            
            if((NULL != t->node->mdo)
                || (OK == IoBuildDeviceStack(t->node)))
            {
                if((IO_DEVICE_TYPE_BUS == t->node->mdo->type)
                    || (t->node->mdo->flags & IO_DEVICE_FLAG_ENUMERATION_CAPABLE))
                {
                    struct IoRp *rp;
                    if(OK == IoCreateRp(&rp))
                    {
                        rp->code = IO_RP_ENUMERATE;
                        rp->flags |= IO_DRIVER_RP_FLAG_SYNCHRONOUS;
                        IoSendRp(t->node->mdo, rp);
                        
                        MmFreeKernelHeap(rp);
                    }
                }
                // if(t->device->type == IO_DEVICE_TYPE_DISK)
                // {
                //     struct IoRp *rp;
                //     IoCreateRp(&rp);
                //     rp->device = IoGetDeviceStackStop(t->device);
                //     rp->code = IO_RP_READ;
                //     rp->flags = 0;
                //     rp->completionCallback = cb;
                //     struct IoMemoryDescriptor *mem;
                //     mem = MmAllocateKernelHeap(sizeof(*mem));
                //     mem->next = NULL;
                //     MmAllocateContiguousPhysicalMemory(1024, &mem->physical, 2);
                //     mem->size = 1024;
                //     mem->mapped = MmMapMmIo(mem->physical, 1024);
                //     rp->payload.readWrite.memory = mem;
                //     rp->completionContext = mem;
                //     rp->size = 512;
                //     rp->payload.readWrite.offset = 0;
                //     IoSendRp(NULL, t->device, rp);
                // }
            }
            MmFreeKernelHeap(t);
            
            KeAcquireSpinlock(&IoEnumerationQueueMutex);
        }
        KeReleaseSpinlock(&IoEnumerationQueueMutex); 

        KeBlockTask(enumeratorThread, TASK_BLOCK_EVENT);
        KeTaskYield();
    }
}

STATUS IoStartDeviceEnumerationThread(void)
{
    STATUS status;
    //TODO: THREAD! Not a process. No threads implemented yet, though.
    if(OK != (status = KeCreateProcessRaw("Device enumerator", NULL, PL_KERNEL, IoEnumeratorThread, NULL, &enumeratorThread)))
        return status;
    KeChangeTaskMajorPriority(enumeratorThread, TCB_DEFAULT_MAJOR_PRIORITY);
    KeChangeTaskMinorPriority(enumeratorThread, TCB_DEFAULT_MINOR_PRIORITY);
    KeEnableTask(enumeratorThread);
    return OK;
}

STATUS IoNotifyDeviceEnumerator(struct IoDeviceNode *node)
{
    struct IoEnumerationQueue *t = MmAllocateKernelHeap(sizeof(*t));
    if(NULL == t)
        return OUT_OF_RESOURCES;

    t->node = node;
    t->next = NULL;
    KeAcquireSpinlock(&IoEnumerationQueueMutex);
    if(NULL == IoEnumerationQueueHead)
    {
        IoEnumerationQueueHead = t;
    }
    else
    {
        struct IoEnumerationQueue *last = IoEnumerationQueueHead;
        while(last->next)
            last = last->next;
        last->next = t;
    }
    KeReleaseSpinlock(&IoEnumerationQueueMutex);
    KeUnblockTask(enumeratorThread);
    return OK;
}