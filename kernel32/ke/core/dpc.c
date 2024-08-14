#include "dpc.h"
#include <stdbool.h>
#include "common.h"
#include "hal/time.h"
#include "hal/interrupt.h"
#include "mutex.h"
#include "panic.h"
#include "ke/sched/sched.h"
#include "mm/slab.h"

#define KE_DPC_CHUNK_PER_SLAB 64

struct KeDpcObject
{
    //caller provided data
    enum KeDpcPriority priority; //DPC priority
    KeDpcCallback callback; //DPC entry point
    void *context; //callback context

    uint64_t time; //time of creation, then subtracted from the time of execution start to obtain latency
    struct KeDpcObject *next; //next DPC on the list
};

static struct
{
    void *slabHandle;
    struct KeDpcObject *queue[_KE_DPC_PRIORITY_LIMIT + 1];
    bool isPending;
    bool isOngoing;
    KeSpinlock lock;
} 
KeDpcState = {.slabHandle = NULL, .queue[0] = NULL, .queue[1] = NULL, .queue[2] = NULL, 
                .isPending = false, .isOngoing = false, .lock = KeSpinlockInitializer};

STATUS KeRegisterDpc(enum KeDpcPriority priority, KeDpcCallback callback, void *context)
{
    HalCheckPriorityLevel(HAL_PRIORITY_LEVEL_DPC, HAL_PRIORITY_LEVEL_EXCLUSIVE);

    uint8_t queueIndex = 0;
    switch(priority)
    {
        case KE_DPC_PRIORITY_LOW:
            queueIndex = 2;
            break;
        case KE_DPC_PRIORITY_NORMAL:
            queueIndex = 1;
            break;
        case KE_DPC_PRIORITY_HIGH:
            queueIndex = 0;
            break;
        default:
            return BAD_PARAMETER;
            break;
    }

    struct KeDpcObject *dpc = MmSlabAllocate(KeDpcState.slabHandle);
    if(NULL == dpc)
        return OUT_OF_RESOURCES;

    dpc->callback = callback;
    dpc->context = context;
    dpc->priority = priority;
    dpc->next = NULL;
    
    PRIO prio = KeAcquireSpinlock(&(KeDpcState.lock));
    if(NULL == KeDpcState.queue[queueIndex])
        KeDpcState.queue[queueIndex] = dpc;
    else
    {
        struct KeDpcObject *t = KeDpcState.queue[queueIndex];
        while(NULL != t->next)
        {
            t = t->next;
        }
        t->next = dpc;
    }
    dpc->time = HalGetTimestamp();
    if(false == KeDpcState.isOngoing)
        KeDpcState.isPending = true;
    KeReleaseSpinlock(&(KeDpcState.lock), prio);
    return OK;
}

static void KeDpcProcess(void)
{
    PRIO prio = KeAcquireSpinlock(&(KeDpcState.lock));
    KeDpcState.isOngoing = true;
    KeDpcState.isPending = false;
    for(uint8_t i = 0; i < (_KE_DPC_PRIORITY_LIMIT + 1); i++)
    {
        struct KeDpcObject *t = KeDpcState.queue[i];
        while(NULL != KeDpcState.queue[i])
        {
            t = KeDpcState.queue[i];
            KeReleaseSpinlock(&(KeDpcState.lock), prio);
            t->time = HalGetTimestamp() - t->time;
            t->callback(t->context);
            prio = KeAcquireSpinlock(&(KeDpcState.lock));
            KeDpcState.queue[i] = t->next;
            MmSlabFree(KeDpcState.slabHandle, t);
        }
    }
    KeDpcState.isOngoing = false;
    KeReleaseSpinlock(&(KeDpcState.lock), prio);
}

void KeProcessDpcQueue(void)
{
    if(HalGetProcessorPriority() > HAL_PRIORITY_LEVEL_DPC)
        return;
    PRIO prio = KeAcquireSpinlock(&(KeDpcState.lock));
    if(KeDpcState.isPending)
    {
        KeReleaseSpinlock(&(KeDpcState.lock), prio);
        KeDpcProcess();
        KePerformPreemptedTaskSwitch();
        return;
    }
    KeReleaseSpinlock(&(KeDpcState.lock), prio);
}

STATUS KeDpcInitialize(void)
{
    KeDpcState.slabHandle = MmSlabCreate(sizeof(struct KeDpcObject), KE_DPC_CHUNK_PER_SLAB);
    if(NULL == KeDpcState.slabHandle)
        return OUT_OF_RESOURCES;
    
    return OK;
}