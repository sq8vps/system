#include "idle.h"
#include "ke/task/task.h"
#include "sched.h"
#include "hal/arch.h"

static struct KeProcessControlBlock *KeIdlePCB = NULL;

NORETURN static void KeIdleWorker(void *unused)
{
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
        ret = KeCreateKernelProcess("Idle task", KE_TASK_FLAG_IDLE | KE_TASK_FLAG_CRITICAL, KeIdleWorker, NULL, &tcb);
        if(OK != ret)
            return ret;
        KeIdlePCB = tcb->parent;
    }
    else
    {
        ret = KeCreateKernelThread(KeIdlePCB, "Idle task", KE_TASK_FLAG_IDLE | KE_TASK_FLAG_CRITICAL, KeIdleWorker, NULL, &tcb);
        if(OK != ret)
            return ret;
    }
    
    //set lowest possible priority for this task
    if(OK != (ret = KeChangeTaskMajorPriority(tcb, PRIORITY_LOWEST)))
        return ret;

    if(OK != (ret = KeChangeTaskMinorPriority(tcb, TCB_MINOR_PRIORITY_LIMIT)))
        return ret;

    return KeEnableTask(tcb);
}