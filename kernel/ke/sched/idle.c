#include "idle.h"
#include "ke/task/task.h"
#include "sched.h"
#include "hal/arch.h"

static struct KeProcessControlBlock *KeIdlePCB = NULL;

[[noreturn]] static void KeIdleWorker(void *context)
{
    UNUSED(context);

    while(1)
    {
        //KeTaskYield();
        HALT();
    }
}

STATUS KeCreateIdleTask(void)
{
    STATUS ret = OK;
    struct KeTaskControlBlock *tcb = NULL;
    if(NULL == KeIdlePCB)
    {
        ret = KeCreateKernelProcess(KE_TASK_FLAG_IDLE | KE_TASK_FLAG_CRITICAL, KeIdleWorker, NULL, NULL, &tcb);
        if(OK != ret)
            return ret;
        KeIdlePCB = tcb->parent;
    }
    else
    {
        ret = KeCreateKernelThread(KeIdlePCB, KE_TASK_FLAG_IDLE | KE_TASK_FLAG_CRITICAL, KeIdleWorker, NULL, &tcb);
        if(OK != ret)
            return ret;
    }
    
    

    return KeEnableTask(tcb);
}