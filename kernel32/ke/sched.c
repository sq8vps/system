#include "sched.h"
#include <stdbool.h>
#include "../it/it.h"
#include "panic.h"
#include "idle.h"
#include "../common.h"
#include "../hal/hal.h"
#include "mutex.h"

static uint32_t postponeCounter = 0; //count of scheduler task switch postponement events
//static bool postponedTaskPending = false; //is there a postponed task switch to perform?

#ifdef SMP
    volatile struct TaskControlBlock *currentTask[MAX_CPU_COUNT];
    struct TaskControlBlock *queue[MAX_CPU_COUNT][PRIORITY_LOWEST + 1][TCB_MINOR_PRIORITY_LIMIT + 1];
#else
    struct TaskControlBlock *currentTask; //current task TCB
    struct TaskControlBlock **currentQueueHead; //current queue head
    struct TaskControlBlock *readyToRun[PRIORITY_LOWEST + 1][TCB_MINOR_PRIORITY_LIMIT + 1]; //ready to run tasks
    struct TaskControlBlock *terminated; //terminated tasks
#endif

extern void KeSchedulerISR(struct ItFrame *f);

void KeSchedulerISRClearFlag()
{
    HalClearInterruptFlag(32);
}

void KeSchedule(void)
{
    if(postponeCounter) //is scheduler operation postponed?
    {
        //postponedTaskPending = true; //mark that the scheduler was invoked
        //extend time slice
        return; //return for now
    }

    if(TASK_RUNNING == currentTask->state)
        KeChangeTaskState(currentTask, TASK_READY_TO_RUN);

    //update next task in queue
    if(NULL != *currentQueueHead)
    {
        *currentQueueHead = (*currentQueueHead)->next;
    }

    for(uint16_t major = 0; major < (PRIORITY_LOWEST + 1); major++)
    {
        for(uint16_t minor = 0; minor < (TCB_MINOR_PRIORITY_LIMIT + 1); minor++)
        {
            if(NULL != readyToRun[major][minor])
            {
                currentTask = readyToRun[major][minor];
                currentQueueHead = &readyToRun[major][minor];

                KeChangeTaskState(currentTask, TASK_RUNNING);
                return;
            }
        }
    }
    //should never reach this point
    KePanic(KE_NO_EXECUTABLE_TASK);
}

void KeSchedulerIncrementPostponeCounter(void)
{
    asm volatile("lock inc %0" : "=m" (postponeCounter) : );
}

void KeSchedulerDecrementPostponeCounter(void)
{
    if(0 == postponeCounter) //nothing to decrement
        return;

    asm volatile("lock dec %0" : "=m" (postponeCounter) : );
    // asm volatile("jnz .stillPostponed");
    // if(postponedTaskPending) //counter is 0 and there is a postponed task switch?
    // {
    //     postponedTaskPending = false;
    //     //invoke scheduler now
    //     KeSchedule();
    // }
    // asm volatile(".stillPostponed:");
}


NORETURN void KeSchedulerStart(void)
{
    Cm_memset(readyToRun, 0, sizeof(readyToRun));
    terminated = NULL;
    currentQueueHead = &readyToRun[PRIORITY_LOWEST][TCB_MINOR_PRIORITY_LIMIT];
    
    //create dummy task that is needed to perform first task switch to an actual task
    uintptr_t cr3, esp;
    ASM("mov %0,cr3" : "=r" (cr3) : );
    ASM("mov %0,esp" : "=r" (esp) : );
    currentTask = KePrepareTCB(esp, esp, cr3, PL_KERNEL, "", NULL);
    if(NULL == currentTask)
        KePanic(KE_SCHEDULER_INITIALIZATION_FAILURE);

    KeChangeTaskState(currentTask, TASK_TERMINATED);

    STATUS ret = OK;
    //create idle task
    if(OK != (ret = KeCreateIdleTask()))
        KePanicEx(KE_SCHEDULER_INITIALIZATION_FAILURE, ret, 0, 0, 0);

    It_setInterruptHandler(32, KeSchedulerISR, PL_KERNEL);
	HalEnableInterrupt(32);

    while(1);;
}

static KeSpinLock_t queueAppendMutex;

static void appendToQueue(struct TaskControlBlock *tcb, struct TaskControlBlock **queueHandle)
{
    //update neighboring tasks
    if(NULL != tcb->previous)
        tcb->previous->next = tcb->next;
    
    if(NULL != tcb->next)
        tcb->next->previous = tcb->previous;

    //the queue is empty
    //add immediately
    if(NULL == *queueHandle)
    {
        tcb->previous = tcb;
        tcb->next = tcb;
        *queueHandle = tcb;
        return;
    }

    //else append task to the end
    tcb->next = *queueHandle;
    tcb->previous = (*queueHandle)->previous;
    (*queueHandle)->previous = tcb;
}

STATUS KeChangeTaskMajorPriority(struct TaskControlBlock *tcb, enum TaskMajorPriority priority)
{
    if(NULL == tcb)
        return NULL_POINTER_GIVEN;
    tcb->majorPriority = priority;

    if(TASK_READY_TO_RUN != tcb->state)
        return OK; //task that is not ready-to-run does not belong to any prioritized queue
    
    appendToQueue(tcb, &readyToRun[tcb->majorPriority][tcb->minorPriority]);

    return OK;
}

STATUS KeChangeTaskMinorPriority(struct TaskControlBlock *tcb, uint8_t priority)
{
    if(NULL == tcb)
        return NULL_POINTER_GIVEN;
    if(priority > TCB_MINOR_PRIORITY_LIMIT)
        priority = TCB_MINOR_PRIORITY_LIMIT;

    tcb->minorPriority = priority;

    if(TASK_READY_TO_RUN != tcb->state)
        return OK; //task that is not ready-to-run does not belong to any prioritized queue
    
    appendToQueue(tcb, &readyToRun[tcb->majorPriority][tcb->minorPriority]);

    return OK;
}

STATUS KeChangeTaskState(struct TaskControlBlock *tcb, enum TaskState state)
{
    if(NULL == tcb)
        return NULL_POINTER_GIVEN;

    tcb->state = state;

    switch(state)
    {
        case TASK_UNINITIALIZED:
        case TASK_RUNNING:
            tcb->previous = NULL;
            tcb->next = NULL;
            break;
        case TASK_READY_TO_RUN:
            appendToQueue(tcb, &readyToRun[tcb->majorPriority][tcb->minorPriority]);
            break;
        case TASK_TERMINATED:
            appendToQueue(tcb, &terminated);
            break;
        case TASK_WAITING:
            break;
    }
    
    return OK;
}