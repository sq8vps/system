#include "idle.h"
#include "task.h"
#include "sched.h"
#include "mm/valloc.h"

NORETURN static void KeIdleWorker(void)
{
    while(1)
    {
        ASM("hlt");
    }
}

STATUS KeCreateIdleTask(void)
{
    STATUS ret = OK;
    struct TaskControlBlock *tcb = NULL;
    if(OK != (ret = KeCreateProcessRaw("Idle task", NULL, PL_KERNEL, KeIdleWorker, &tcb)))
        return ret;
    
    //set lowest possible priority for this task
    KeChangeTaskMajorPriority(tcb, PRIORITY_LOWEST);
    KeChangeTaskMinorPriority(tcb, TCB_MINOR_PRIORITY_LIMIT);

    KeEnableTask(tcb);

    return OK;
}