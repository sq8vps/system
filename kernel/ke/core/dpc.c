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
    } queue[KE_DPC_PRIORITY_COUNT];
    bool isPending;
}
#ifndef SMP
KeDpcState[1];
#define KE_DPC_STATE_SHARED 0
#else
KeDpcState[MAX_CPU_COUNT + 1];
#define KE_DPC_STATE_SHARED MAX_CPU_COUNT
#endif

STATUS KeRegisterDpc(enum KeDpcPriority priority, KeDpcCallback callback, void *context, bool cpuBound)
{
    HalCheckPriorityLevel(HAL_PRIORITY_LEVEL_DPC, HAL_PRIORITY_LEVEL_EXCLUSIVE);
    uint32_t cpu = 0;
    SMP_ONLY(cpu = HalGetCurrentCpu());
    const size_t cpuIndex = cpuBound ? cpu : KE_DPC_STATE_SHARED;
    size_t prioIndex = 0;
    switch(priority)
    {
        case KE_DPC_PRIORITY_LOW:
            prioIndex = 2;
            break;
        case KE_DPC_PRIORITY_NORMAL:
            prioIndex = 1;
            break;
        case KE_DPC_PRIORITY_HIGH:
            prioIndex = 0;
            break;
        default:
            return BAD_PARAMETER;
            break;
    }

    struct KeDpcObject *dpc = MmSlabAllocate(KeDpcState[cpuIndex].slabHandle);
    if(NULL == dpc)
        return OUT_OF_RESOURCES;

    dpc->callback = callback;
    dpc->context = context;
    dpc->priority = priority;
    dpc->next = NULL;
    
    PRIO prio = KeAcquireSpinlock(&(KeDpcState[cpuIndex].queue[prioIndex].lock));
    if(NULL == KeDpcState[cpuIndex].queue[prioIndex].head)
        KeDpcState[cpuIndex].queue[prioIndex].head = dpc;
    else
    {
        struct KeDpcObject *t = KeDpcState[cpuIndex].queue[prioIndex].head;
        while(NULL != t->next)
        {
            t = t->next;
        }
        t->next = dpc;
    }
    KeReleaseSpinlock(&(KeDpcState[cpuIndex].queue[prioIndex].lock), prio);

    dpc->time = HalGetTimestamp();

    ATOMIC_STORE(&(KeDpcState[cpuIndex].isPending), true, ATOMIC_RELEASE);

    return OK;
}

static void KeDpcProcess(uint32_t cpu)
{
    while(ATOMIC_EXCHANGE(&(KeDpcState[cpu].isPending), false, ATOMIC_SEQ_CST))
    {
        for(uint8_t i = 0; i < KE_DPC_PRIORITY_COUNT; i++)
        {
            PRIO prio = KeAcquireDpcLevelSpinlock(&(KeDpcState[cpu].queue[i].lock));
            struct KeDpcObject *t = KeDpcState[cpu].queue[i].head;
            while(NULL != KeDpcState[cpu].queue[i].head)
            {
                t = KeDpcState[cpu].queue[i].head;
                KeDpcState[cpu].queue[i].head = t->next;
                KeReleaseSpinlock(&(KeDpcState[cpu].queue[i].lock), prio);
                t->time = HalGetTimestamp() - t->time;
                t->callback(t->context);
                MmSlabFree(KeDpcState[cpu].slabHandle, t);
                prio = KeAcquireDpcLevelSpinlock(&(KeDpcState[cpu].queue[i].lock));
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
    uint32_t cpu = 0;
    SMP_ONLY(cpu = HalGetCurrentCpu());
    if(ATOMIC_LOAD(&(KeDpcState[cpu].isPending), ATOMIC_RELAXED) 
        SMP_ONLY(|| ATOMIC_LOAD(&(KeDpcState[KE_DPC_STATE_SHARED].isPending), ATOMIC_RELAXED)))
    {
        KeDpcProcess(cpu);
        SMP_ONLY(KeDpcProcess(KE_DPC_STATE_SHARED));
        HalLowerPriorityLevel(dpcPrio);
        KePerformTaskSwitch();
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
    for(size_t i = 0; i < MAX_CPU_COUNT + 1; i++)
    {
        KeDpcState[i].slabHandle = MmSlabCreate(sizeof(struct KeDpcObject), KE_DPC_CHUNK_PER_SLAB);
        if(NULL == KeDpcState[i].slabHandle)
            return OUT_OF_RESOURCES;       
    }
#endif
    return OK;
}