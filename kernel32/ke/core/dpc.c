#include "dpc.h"
#include <stdbool.h>
#include "hal/hal.h"
#include "hal/time.h"
#include "hal/interrupt.h"
#include "mutex.h"
#include "panic.h"
#include "ke/sched/sched.h"
#include "mm/slab.h"
#include "hal/arch.h"
#include "assert.h"
#include "rtl/string.h"
#include "hal/task.h"

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
    struct
    {
        struct KeDpcObject *head;
        KeSpinlock lock;
    } queue[_KE_DPC_PRIORITY_LIMIT + 1];
    bool isPending;
}
#ifndef SMP
KeDpcState[1];
#else
KeDpcState[MAX_CPU_COUNT];
#endif

STATUS KeRegisterDpc(enum KeDpcPriority priority, KeDpcCallback callback, void *context)
{
    HalCheckPriorityLevel(HAL_PRIORITY_LEVEL_DPC, HAL_PRIORITY_LEVEL_EXCLUSIVE);
    uint16_t cpu = 0;
#ifdef SMP
    cpu = HalGetCurrentCpu();
#endif
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

    struct KeDpcObject *dpc = MmSlabAllocate(KeDpcState[cpu].slabHandle);
    if(NULL == dpc)
        return OUT_OF_RESOURCES;

    dpc->callback = callback;
    dpc->context = context;
    dpc->priority = priority;
    dpc->next = NULL;
    
    PRIO prio = KeAcquireSpinlock(&(KeDpcState[cpu].queue[queueIndex].lock));
    if(NULL == KeDpcState[cpu].queue[queueIndex].head)
        KeDpcState[cpu].queue[queueIndex].head = dpc;
    else
    {
        struct KeDpcObject *t = KeDpcState[cpu].queue[queueIndex].head;
        while(NULL != t->next)
        {
            t = t->next;
        }
        t->next = dpc;
    }
    KeReleaseSpinlock(&(KeDpcState[cpu].queue[queueIndex].lock), prio);

    dpc->time = HalGetTimestamp();

    __atomic_store_n(&(KeDpcState[cpu].isPending), true, __ATOMIC_SEQ_CST);

    return OK;
}

static void KeDpcProcess(uint16_t cpu)
{
    while(__atomic_load_n(&(KeDpcState[cpu].isPending), __ATOMIC_SEQ_CST))
    {
        __atomic_store_n(&(KeDpcState[cpu].isPending), false, __ATOMIC_SEQ_CST);
        for(uint8_t i = 0; i < (_KE_DPC_PRIORITY_LIMIT + 1); i++)
        {
            PRIO prio = KeAcquireSpinlock(&(KeDpcState[cpu].queue[i].lock));
            struct KeDpcObject *t = KeDpcState[cpu].queue[i].head;
            while(NULL != KeDpcState[cpu].queue[i].head)
            {
                t = KeDpcState[cpu].queue[i].head;
                KeDpcState[cpu].queue[i].head = t->next;
                KeReleaseSpinlock(&(KeDpcState[cpu].queue[i].lock), prio);
                t->time = HalGetTimestamp() - t->time;
                t->callback(t->context);
                MmSlabFree(KeDpcState[cpu].slabHandle, t);
                prio = KeAcquireSpinlock(&(KeDpcState[cpu].queue[i].lock));
            }
            KeReleaseSpinlock(&(KeDpcState[cpu].queue[i].lock), prio);
        }
    }
}

void KeProcessDpcQueue(void)
{
    if(HalGetProcessorPriority() > HAL_PRIORITY_LEVEL_PASSIVE)
        return;
    PRIO dpcPrio = HalRaisePriorityLevel(HAL_PRIORITY_LEVEL_DPC);
    uint16_t cpu = 0;
#ifdef SMP
        cpu = HalGetCurrentCpu();
#endif
    if(__atomic_load_n(&(KeDpcState[cpu].isPending), __ATOMIC_SEQ_CST))
    {
        KeDpcProcess(cpu);
        HalLowerPriorityLevel(dpcPrio);
        HalPerformTaskSwitch();
        return;
    }
    HalLowerPriorityLevel(dpcPrio);
}

STATUS KeDpcInitialize(void)
{
    RtlMemset(KeDpcState, 0, sizeof(KeDpcState));
#ifndef SMP
    KeDpcState[0].slabHandle = MmSlabCreate(sizeof(struct KeDpcObject), KE_DPC_CHUNK_PER_SLAB);
    if(NULL == KeDpcState[0].slabHandle)
        return OUT_OF_RESOURCES;
#else
    for(uint16_t i = 0; i < MAX_CPU_COUNT; i++)
    {
        KeDpcState[i].slabHandle = MmSlabCreate(sizeof(struct KeDpcObject), KE_DPC_CHUNK_PER_SLAB);
        if(NULL == KeDpcState[i].slabHandle)
            return OUT_OF_RESOURCES;       
    }
#endif
    return OK;
}