#include "worker.h"
#include "ke/task/task.h"
#include "ke/sched/sched.h"
#include "io/dev/dev.h"
#include "ke/core/mutex.h"
#include "mm/heap.h"
#include "io/dev/rp.h"

static struct KeTaskControlBlock *ExKernelWorker = NULL;
static KeSpinlock ExKernelWorkerLock = KeSpinlockInitializer;

STATUS ExCreateKernelWorker(const char *name, void(*entry)(void *), void *entryContext, struct KeTaskControlBlock **tcb)
{
    STATUS status;
    PRIO prio = KeAcquireSpinlock(&ExKernelWorkerLock);
    if(NULL == ExKernelWorker)
    {
        status = KeCreateProcessRaw(name, NULL, PL_KERNEL, entry, entryContext, &ExKernelWorker);
        *tcb = ExKernelWorker;
    }
    else
        status = KeCreateThread(ExKernelWorker, name, entry, entryContext, tcb);
    barrier();
    KeReleaseSpinlock(&ExKernelWorkerLock, prio);

    if(NULL != *tcb)
    {

        KeChangeTaskMajorPriority(*tcb, TCB_DEFAULT_MAJOR_PRIORITY);
        KeChangeTaskMinorPriority(*tcb, TCB_DEFAULT_MINOR_PRIORITY);
        KeEnableTask(*tcb);
    }
    return status;
}

