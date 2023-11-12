#include "sched.h"
#include <stdbool.h>
#include "ke/core/panic.h"
#include "idle.h"
#include "common.h"
#include "hal/hal.h"
#include "hal/interrupt.h"
#include "ke/core/mutex.h"
#include "it/it.h"
#include "sleep.h"

#define KE_SCHEDULER_TIME_SLICE 10000 //microseconds

uint32_t KeSchedPostponeCounter = 0; //count of scheduler task switch postponement events
bool KeSchedTaskSwitchPending = false; //is there a task switch to perform?

struct KeSchedulerQueue
{
    struct KeTaskControlBlock *head;
    KeSpinlock spinlock;
};

#ifdef SMP
    volatile struct KeTaskControlBlock *currentTask[MAX_CPU_COUNT];
    struct KeTaskControlBlock *queue[MAX_CPU_COUNT][PRIORITY_LOWEST + 1][TCB_MINOR_PRIORITY_LIMIT + 1];
#else
    struct KeTaskControlBlock *currentTask; //current task TCB
    static struct KeTaskControlBlock *readyToRun[PRIORITY_LOWEST + 1][TCB_MINOR_PRIORITY_LIMIT + 1]; //next ready to run task
    static struct KeTaskControlBlock *terminated; //next task to be terminated
#endif

extern void KeSchedulerISR(struct ItFrame *f);

void KeSchedulerISRClearFlag()
{
    HalClearInterruptFlag(IT_SYSTEM_TIMER_VECTOR);
}

static KeSpinlock queueMutex = KeSpinlockInitializer;

/**
 * @brief Detach task from current queue
 * @param *tcb TCB pointer
*/
static void detachFromQueue(struct KeTaskControlBlock *tcb)
{
    KeAcquireSpinlock(&queueMutex);
    if(tcb->next == tcb)
    {
        tcb->next = NULL;
    }
    else
    {
        tcb->next->previous = tcb->previous;
        tcb->previous->next = tcb->next;
    }

    if((NULL != tcb->queue) && (tcb == *(tcb->queue)))
    {
        *(tcb->queue) = tcb->next;
        if(NULL != tcb->next)
            tcb->next->queue = tcb->queue;
    }
    tcb->next = NULL;
    tcb->previous = NULL;
    tcb->queue = NULL;
    KeReleaseSpinlock(&queueMutex);
}

/**
 * @brief Attach task to given queue
 * @param *tcb TCB pointer
 * @param **queueHandle Queue head pointer
 * @warning This function does NOT detach the task from previous queue
*/
static void attachToQueue(struct KeTaskControlBlock *tcb, struct KeTaskControlBlock **queueHandle)
{
    KeAcquireSpinlock(&queueMutex);
    if(NULL == *queueHandle)
    {
        *queueHandle = tcb;
        tcb->next = tcb;
        tcb->previous = tcb;
        tcb->queue = queueHandle;
        KeReleaseSpinlock(&queueMutex);
        return;
    }

    tcb->previous = (*queueHandle)->previous;
    tcb->next = *queueHandle;
    (*queueHandle)->previous->next = tcb;
    (*queueHandle)->previous = tcb;
    tcb->queue = NULL;
    KeReleaseSpinlock(&queueMutex);
}

static KeSpinlock schedulerMutex = KeSpinlockInitializer;

void KeSchedule(void)
{
    KeAcquireSpinlock(&schedulerMutex);
    
    //reattach task to appropriate queue
    switch(currentTask->requestedState)
    {
        case TASK_READY_TO_RUN:
        case TASK_RUNNING:
            detachFromQueue(currentTask);
            currentTask->state = TASK_READY_TO_RUN;
            attachToQueue(currentTask, &readyToRun[currentTask->majorPriority][currentTask->minorPriority]);
            break;
        case TASK_TERMINATED:
        case TASK_UNINITIALIZED:
            detachFromQueue(currentTask);
            currentTask->state = TASK_TERMINATED;
            attachToQueue(currentTask, &terminated);
            break;
        case TASK_WAITING:
            //the task should be already detached by KeBlockTask()
            currentTask->state = TASK_WAITING;
            break;
    }

    KeRefreshSleepingTasks();
    KeTimedExclusionRefresh();

    for(uint16_t major = 0; major < (PRIORITY_LOWEST + 1); major++)
    {
        for(uint16_t minor = 0; minor < (TCB_MINOR_PRIORITY_LIMIT + 1); minor++)
        {
            if(NULL != readyToRun[major][minor])
            {
                //get current task from queue
                currentTask = readyToRun[major][minor];
                //update state
                currentTask->state = TASK_RUNNING;
                KeReleaseSpinlock(&schedulerMutex);
                HalStartSystemTimer(KE_SCHEDULER_TIME_SLICE);
                return;
            }
        }
    }
    //should never reach this point
    KePanic(NO_EXECUTABLE_TASK);
}

void KeSchedulerIncrementPostponeCounter(void)
{
    ASM("lock inc %0" : "=m" (KeSchedPostponeCounter) : );
}

void KeSchedulerDecrementPostponeCounter(void)
{
    if(0 == KeSchedPostponeCounter)
        return;

    ASM("lock dec %0" : "=m" (KeSchedPostponeCounter) : );
    ASM("jnz .stillPostponed");
    if(KeSchedTaskSwitchPending) //counter is 0 and there is a postponed task switch?
    {
        KeSchedTaskSwitchPending = false;
        HalStartSystemTimer(KE_SCHEDULER_TIME_SLICE);
        KeTaskYield();
    }
    ASM(".stillPostponed:");
}


void KeSchedulerStart(void)
{
    CmMemset(readyToRun, 0, sizeof(readyToRun));
    terminated = NULL;
    
    STATUS ret = OK;
    //create idle task
    if(OK != (ret = KeCreateIdleTask()))
        KePanicEx(BOOT_FAILURE, KE_SCHEDULER_INITIALIZATION_FAILURE, ret, 0, 0);

    ItInstallInterruptHandler(IT_SYSTEM_TIMER_VECTOR, (ItHandler)KeSchedulerISR, NULL, PL_KERNEL);

    //create kernel initialization task
    uintptr_t cr3;
    ASM("mov %0,cr3" : "=r" (cr3) : );
    //this is the very first task, so stack pointers are not needed
    //there will be filled at the first task context store event
    struct KeTaskControlBlock *tcb = KePrepareTCB(0, 0, cr3, PL_KERNEL, "KernelInit", NULL);
    if(NULL == tcb)
        KePanicEx(BOOT_FAILURE, KE_SCHEDULER_INITIALIZATION_FAILURE, KE_TCB_PREPARATION_FAILURE, 0, 0);
    
    KeChangeTaskMajorPriority(tcb, TCB_DEFAULT_MAJOR_PRIORITY);
    KeChangeTaskMinorPriority(tcb, TCB_DEFAULT_MINOR_PRIORITY);
    KeEnableTask(tcb);

    currentTask = tcb;

    HalConfigureSystemTimer(IT_SYSTEM_TIMER_VECTOR);
    HalStartSystemTimer(KE_SCHEDULER_TIME_SLICE);
	HalEnableIRQ(IT_SYSTEM_TIMER_VECTOR);

    while(NULL == KeGetCurrentTask())
        ;
}


STATUS KeChangeTaskMajorPriority(struct KeTaskControlBlock *tcb, enum KeTaskMajorPriority priority)
{
    if(NULL == tcb)
        return NULL_POINTER_GIVEN;

    tcb->majorPriority = priority;

    if(TASK_READY_TO_RUN != tcb->state)
        return OK; //task that is not ready-to-run does not belong to any prioritized queue
    
    detachFromQueue(tcb);
    attachToQueue(tcb, &readyToRun[tcb->majorPriority][tcb->minorPriority]);

    return OK;
}

STATUS KeChangeTaskMinorPriority(struct KeTaskControlBlock *tcb, uint8_t priority)
{
    if(NULL == tcb)
        return NULL_POINTER_GIVEN;

    if(priority > TCB_MINOR_PRIORITY_LIMIT)
        priority = TCB_MINOR_PRIORITY_LIMIT;

    tcb->minorPriority = priority;

    if(TASK_READY_TO_RUN != tcb->state)
        return OK; //task that is not ready-to-run does not belong to any prioritized queue
    
    detachFromQueue(tcb);
    attachToQueue(tcb, &readyToRun[tcb->majorPriority][tcb->minorPriority]);

    return OK;
}

STATUS KeEnableTask(struct KeTaskControlBlock *tcb)
{
    if(NULL == tcb)
        return NULL_POINTER_GIVEN;

    tcb->requestedState = TASK_READY_TO_RUN;
    attachToQueue(tcb, &readyToRun[tcb->majorPriority][tcb->minorPriority]);
    
    return OK;
}

void KeBlockTask(struct KeTaskControlBlock *tcb, enum KeTaskBlockReason reason)
{
    tcb->requestedState = TASK_WAITING;
    tcb->blockReason = reason;
    detachFromQueue(tcb);
}

void KeUnblockTask(struct KeTaskControlBlock *tcb)
{
    tcb->requestedState = TASK_READY_TO_RUN;
    tcb->blockReason = TASK_BLOCK_NOT_BLOCKED;
    attachToQueue(tcb, &readyToRun[tcb->majorPriority][tcb->minorPriority]);
}

struct KeTaskControlBlock* KeGetCurrentTask(void)
{
    return currentTask;
}