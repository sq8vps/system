#include "idle.h"
#include "ke/task/task.h"
#include "sched.h"

NORETURN static void KeIdleWorker(void *unused)
{
    while(1)
    {
        HALT();
    }
}

STATUS KeCreateIdleTask(void)
{
    STATUS ret = OK;
    struct KeTaskControlBlock *tcb = NULL;
    if(OK != (ret = KeCreateProcessRaw("Idle task", NULL, PL_KERNEL, KeIdleWorker, NULL, &tcb)))
        return ret;
    
    //set lowest possible priority for this task
    if(OK != (ret = KeChangeTaskMajorPriority(tcb, PRIORITY_LOWEST)))
        return ret;

    if(OK != (ret = KeChangeTaskMinorPriority(tcb, TCB_MINOR_PRIORITY_LIMIT)))
        return ret;

    return KeEnableTask(tcb);
}