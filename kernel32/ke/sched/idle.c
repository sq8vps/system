#include "idle.h"
#include "ke/task/task.h"
#include "sched.h"
#include "hal/arch.h"

static struct KeTaskControlBlock *KeIdleTaskMain = NULL;

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
    if(NULL == KeIdleTaskMain)
    {
        ret = KeCreateKernelProcess("Idle task", KeIdleWorker, NULL, &tcb);
        if(OK != ret)
            return ret;
        KeIdleTaskMain = tcb;
    }
    else
    {
        ret = KeCreateThread(KeIdleTaskMain, "Idle task", KeIdleWorker, NULL, &tcb);
        if(OK != ret)
            return ret;
    }

    tcb->flags |= KE_TASK_FLAG_IDLE;
    
    //set lowest possible priority for this task
    if(OK != (ret = KeChangeTaskMajorPriority(tcb, PRIORITY_LOWEST)))
        return ret;

    if(OK != (ret = KeChangeTaskMinorPriority(tcb, TCB_MINOR_PRIORITY_LIMIT)))
        return ret;

    return KeEnableTask(tcb);
}