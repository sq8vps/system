#include "sched.h"
#include <stdbool.h>
#include "panic.h"
#include "idle.h"
#include "common.h"
#include "hal/hal.h"
#include "hal/interrupt.h"
#include "mutex.h"
#include "it/it.h"

#define KE_SCHEDULER_TIME_SLICE 10 //milliseconds

uint32_t KeSchedPostponeCounter = 0; //count of scheduler task switch postponement events
bool KeSchedTaskSwitchPending = false; //is there a task switch to perform?

static uint8_t schedulerTimerVector = 0;

#ifdef SMP
    volatile struct TaskControlBlock *currentTask[MAX_CPU_COUNT];
    struct TaskControlBlock *queue[MAX_CPU_COUNT][PRIORITY_LOWEST + 1][TCB_MINOR_PRIORITY_LIMIT + 1];
#else
    struct TaskControlBlock *currentTask; //current task TCB
    struct TaskControlBlock *readyToRun[PRIORITY_LOWEST + 1][TCB_MINOR_PRIORITY_LIMIT + 1]; //next ready to run task
    struct TaskControlBlock *terminated; //next task to be terminated
#endif

extern void KeSchedulerISR(struct ItFrame *f);

void KeSchedulerISRClearFlag()
{
    HalClearInterruptFlag(schedulerTimerVector);
}

static KeSpinLock_t queueAttachMutex;

/**
 * @brief Attach task to given queue (and detach it from previous queue)
 * @param *tcb TCB pointer
 * @param **queueHandle Queue head pointer
*/
static void attachToQueue(struct TaskControlBlock *tcb, struct TaskControlBlock **queueHandle)
{
    KeAcquireSpinlockDisableIRQ(&queueAttachMutex);
    
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
        KeReleaseSpinlockEnableIRQ(&queueAttachMutex);
        return;
    }

    //else append task to the end
    tcb->next = *queueHandle;
    tcb->previous = (*queueHandle)->previous;
    (*queueHandle)->previous = tcb;
    KeReleaseSpinlockEnableIRQ(&queueAttachMutex);
}

/**
 * @brief Detach task from current queue
 * @param *tcb TCB pointer
*/
static void detachFromQueue(struct TaskControlBlock *tcb)
{
    //update neighboring tasks
    if(NULL != tcb->previous)
        tcb->previous->next = tcb->next;
    
    if(NULL != tcb->next)
        tcb->next->previous = tcb->previous;

    tcb->next = NULL;
    tcb->previous = NULL;
}


static KeSpinLock_t schedulerMutex;

void KeSchedule(void)
{
    KeAcquireSpinlockDisableIRQ(&schedulerMutex);

    //update next task in queue
    if(NULL != currentTask)
    {
        switch(currentTask->state)
        {
            case TASK_RUNNING:
                currentTask->state = TASK_READY_TO_RUN;
                attachToQueue(currentTask, &readyToRun[currentTask->majorPriority][currentTask->minorPriority]);
                break;
            case TASK_TERMINATED:
                attachToQueue(currentTask, &terminated);
                break;
        }
    }

    for(uint16_t major = 0; major < (PRIORITY_LOWEST + 1); major++)
    {
        for(uint16_t minor = 0; minor < (TCB_MINOR_PRIORITY_LIMIT + 1); minor++)
        {
            if(NULL != readyToRun[major][minor])
            {
                //get current task from queue
                currentTask = readyToRun[major][minor];
                //detach it
                detachFromQueue(currentTask);
                //update state
                currentTask->state = TASK_RUNNING;
                //update queue head
                readyToRun[major][minor] = currentTask->next;

                KeReleaseSpinlockEnableIRQ(&schedulerMutex);
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
        HalRestartSystemTimer();
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

    if(IT_METHOD_APIC == HalGetInterruptHandlingMethod())
        schedulerTimerVector = ItGetFreeVector();
    else
        schedulerTimerVector = IT_LEGACY_PIT_VECTOR;
    
    if(0 == schedulerTimerVector)
        KePanicEx(BOOT_FAILURE, KE_SCHEDULER_INITIALIZATION_FAILURE, IT_NO_FREE_VECTORS, 0, 0);

    ItInstallInterruptHandler(schedulerTimerVector, KeSchedulerISR, PL_KERNEL);

    //create kernel initialization task
    uintptr_t cr3;
    ASM("mov %0,cr3" : "=r" (cr3) : );
    //this is the very first task, so stack pointers are not needed
    //there will be filled at the first task context store event
    struct TaskControlBlock *tcb = KePrepareTCB(0, 0, cr3, PL_KERNEL, "KernelInit", NULL);
    if(NULL == tcb)
        KePanicEx(BOOT_FAILURE, KE_SCHEDULER_INITIALIZATION_FAILURE, KE_TCB_PREPARATION_FAILURE, 0, 0);
    
    KeChangeTaskMajorPriority(tcb, TCB_DEFAULT_MAJOR_PRIORITY);
    KeChangeTaskMinorPriority(tcb, TCB_DEFAULT_MINOR_PRIORITY);
    KeEnableTask(tcb);

    currentTask = tcb;

    HalInitSystemTimer(KE_SCHEDULER_TIME_SLICE, schedulerTimerVector);
	HalEnableIRQ(schedulerTimerVector);
}


STATUS KeChangeTaskMajorPriority(struct TaskControlBlock *tcb, enum TaskMajorPriority priority)
{
    if(NULL == tcb)
        return NULL_POINTER_GIVEN;

    tcb->majorPriority = priority;

    if(TASK_READY_TO_RUN != tcb->state)
        return OK; //task that is not ready-to-run does not belong to any prioritized queue
    
    attachToQueue(tcb, &readyToRun[tcb->majorPriority][tcb->minorPriority]);

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
    
    attachToQueue(tcb, &readyToRun[tcb->majorPriority][tcb->minorPriority]);

    return OK;
}

STATUS KeEnableTask(struct TaskControlBlock *tcb)
{
    if(NULL == tcb)
        return NULL_POINTER_GIVEN;

    tcb->state = TASK_READY_TO_RUN;
    attachToQueue(tcb, &readyToRun[tcb->majorPriority][tcb->minorPriority]);
    
    return OK;
}