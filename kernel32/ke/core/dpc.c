#include "dpc.h"
#include <stdbool.h>
#include "mm/heap.h"
#include "common.h"
#include "hal/time.h"
#include "hal/interrupt.h"
#include "mutex.h"
#include "panic.h"
#include "ke/sched/sched.h"

struct KeDpcObject
{
    //caller provided data
    enum KeDpcPriority priority; //DPC priority
    KeDpcCallback callback; //DPC entry point
    void *context; //callback context

    uint64_t time; //time of creation, then subtracted from the time of execution start to obtain latency
    struct KeDpcObject *next; //next DPC on the list
};

static struct KeDpcObject *queue[3] = {NULL, NULL, NULL};
static KeSpinlock queueMutex = KeSpinlockInitializer;

static bool isProcessingOngoing = false, isPending = false;
static KeSpinlock flagMutex = KeSpinlockInitializer;

STATUS KeRegisterDpc(enum KeDpcPriority priority, KeDpcCallback callback, void *context)
{
    if(HalGetProcessorPriority() == HAL_TASK_PRIORITY_PASSIVE)
        KePanicEx(PRIORITY_LEVEL_TOO_LOW, HalGetProcessorPriority(), HAL_TASK_PRIORITY_PASSIVE + 1, 0, 0);

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

    struct KeDpcObject *dpc = MmAllocateKernelHeap(sizeof(struct KeDpcObject));
    if(NULL == dpc)
        return OUT_OF_RESOURCES;
    
    dpc->callback = callback;
    dpc->context = context;
    dpc->priority = priority;
    dpc->next = NULL;
    
    KeAcquireSpinlock(&queueMutex);
    if(NULL == queue[queueIndex])
        queue[queueIndex] = dpc;
    else
    {
        struct KeDpcObject *t = queue[queueIndex];
        while(NULL != t->next)
        {
            t = t->next;
        }
        t->next = dpc;
    }
    dpc->time = HalGetTimestamp();
    KeReleaseSpinlock(&queueMutex);
    KeAcquireSpinlock(&flagMutex);
    if(false == isProcessingOngoing)
        isPending = true;
    HalSetTaskPriority(HAL_TASK_PRIORITY_DPC);
    KeReleaseSpinlock(&flagMutex);
    return OK;
}

static void KeDpcProcess(void)
{
    KeAcquireSpinlock(&flagMutex);
    isProcessingOngoing = true;
    isPending = false;
    KeReleaseSpinlock(&flagMutex);
    for(uint8_t i = 0; i < sizeof(queue) / sizeof(*queue); i++)
    {
        KeAcquireSpinlock(&queueMutex);
        struct KeDpcObject *t = queue[i];
        while(NULL != queue[i])
        {
            t = queue[i];
            KeReleaseSpinlock(&queueMutex);
            t->time = HalGetTimestamp() - t->time;
            t->callback(t->context);
            KeAcquireSpinlock(&queueMutex);
            queue[i] = t->next;
            MmFreeKernelHeap(t);
        }
        KeReleaseSpinlock(&queueMutex);
    }
    KeAcquireSpinlock(&flagMutex);
    isProcessingOngoing = false;
    HalSetTaskPriority(HAL_TASK_PRIORITY_PASSIVE);
    KeReleaseSpinlock(&flagMutex);
}

void KeProcessDpcQueue(void)
{
    KeAcquireSpinlock(&flagMutex);
    if(isPending && (HalGetProcessorPriority() == HAL_TASK_PRIORITY_DPC))
    {
        KeReleaseSpinlock(&flagMutex);
        KeDpcProcess();
        KePerformPreemptedTaskSwitch();
        return;
    }
    KeReleaseSpinlock(&flagMutex);
}

