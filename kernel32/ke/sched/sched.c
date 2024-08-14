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
#include "ke/core/dpc.h"
#include "hal/arch.h"
#include "hal/time.h"

#define KE_SCHEDULER_TIME_SLICE 10000 //microseconds

static bool dpcTaskSwitchPending = false;
static KeSpinlock dpcTaskSwitchFlagMutex = KeSpinlockInitializer;

struct KeSchedulerQueue
{
    struct KeTaskControlBlock *head;
    KeSpinlock spinlock;
};


    struct KeTaskControlBlock *currentTask; //current task TCB
    void *KeCurrentCpuState;
    struct KeTaskControlBlock *nextTask = NULL; //next task TCB planned to be switched after DPC processing
    void *KeNextCpuState = NULL;
    static struct KeTaskControlBlock *readyToRun[PRIORITY_LOWEST + 1][TCB_MINOR_PRIORITY_LIMIT + 1]; //next ready to run task
    static struct KeTaskControlBlock *terminated; //next task to be terminated

static void KeSchedule(void);

//this worker runs always at the DPC level
static void KeSchedulerWorker(void *context)
{
    KeSchedule();
    PRIO prio = KeAcquireSpinlock(&dpcTaskSwitchFlagMutex);
    dpcTaskSwitchPending = true;
    KeReleaseSpinlock(&dpcTaskSwitchFlagMutex, prio);
}

STATUS KeSchedulerISR(void *context)
{
    KeRegisterDpc(KE_DPC_PRIORITY_NORMAL, KeSchedulerWorker, NULL);
    HalStartSystemTimer(KE_SCHEDULER_TIME_SLICE);
    return OK;
}


static KeSpinlock queueMutex = KeSpinlockInitializer;

/**
 * @brief Detach task from current queue
 * @param *tcb TCB pointer
*/
static void detachFromQueue(struct KeTaskControlBlock *tcb)
{
    if(NULL == tcb->queue)
        return;
    PRIO prio = KeAcquireSpinlock(&queueMutex);
    if(tcb != *(tcb->queue))
    {
        tcb->next->previous = tcb->previous;
        tcb->previous->next = tcb->next;
        tcb->next = NULL;
        tcb->previous = NULL;
        tcb->queue = NULL;
    }
    else
    {
        if(tcb->next == tcb)
        {
            *(tcb->queue) = NULL;
            tcb->next = NULL;
            tcb->previous = NULL;
            tcb->queue = NULL;
        }
        else
        {
            tcb->next->previous = tcb->previous;
            tcb->previous->next = tcb->next;
            *(tcb->queue) = tcb->next;
            tcb->next = NULL;
            tcb->previous = NULL;
            tcb->queue = NULL;
        }
    }
    KeReleaseSpinlock(&queueMutex, prio);
}

/**
 * @brief (Re)Attach task to given queue
 * @param *tcb TCB pointer
 * @param **queueHandle Queue head pointer
*/
static void attachToQueue(struct KeTaskControlBlock *tcb, struct KeTaskControlBlock **queueHandle)
{
    if(NULL != tcb->queue)
        detachFromQueue(tcb);
    PRIO prio = KeAcquireSpinlock(&queueMutex);
    if(NULL == *queueHandle)
    {
        tcb->next = tcb;
        tcb->previous = tcb;
        tcb->queue = queueHandle;
        *queueHandle = tcb;
    }
    else
    {
        tcb->next = *queueHandle;
        tcb->previous = (*queueHandle)->previous;
        (*queueHandle)->previous->next = tcb;
        (*queueHandle)->previous = tcb;
        tcb->queue = queueHandle;
    }
    KeReleaseSpinlock(&queueMutex, prio);
}

static KeSpinlock schedulerMutex = KeSpinlockInitializer;

static void KeSchedule(void)
{
    PRIO prio = KeAcquireSpinlock(&schedulerMutex);
    
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
                //get next task from queue
                nextTask = readyToRun[major][minor];
                KeNextCpuState = &(readyToRun[major][minor]->cpu);
                //update state
                nextTask->state = TASK_RUNNING;
                KeReleaseSpinlock(&schedulerMutex, prio);
                HalStartSystemTimer(KE_SCHEDULER_TIME_SLICE);
                return;
            }
        }
    }
    //should never reach this point
    KePanic(NO_EXECUTABLE_TASK);
}

void KeStartScheduler(void)
{
    CmMemset(readyToRun, 0, sizeof(readyToRun));
    terminated = NULL;
    currentTask = NULL;
    KeCurrentCpuState = NULL;
    
    STATUS ret = OK;
    //create idle task
    if(OK != (ret = KeCreateIdleTask()))
        KePanicEx(BOOT_FAILURE, KE_SCHEDULER_INITIALIZATION_FAILURE, ret, 0, 0);

    if(OK != (ret = ItInstallInterruptHandler(IT_SYSTEM_TIMER_VECTOR, KeSchedulerISR, NULL)))
        KePanicEx(BOOT_FAILURE, KE_SCHEDULER_INITIALIZATION_FAILURE, ret, 0, 0);
    if(OK != (ret = ItSetInterruptHandlerEnable(IT_SYSTEM_TIMER_VECTOR, KeSchedulerISR, true)))
        KePanicEx(BOOT_FAILURE, KE_SCHEDULER_INITIALIZATION_FAILURE, ret, 0, 0);
        
    HalInitializeScheduler();

    HalConfigureSystemTimer(IT_SYSTEM_TIMER_VECTOR);
    HalStartSystemTimer(KE_SCHEDULER_TIME_SLICE);
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

enum KeTaskBlockReason KeGetTaskBlockState(struct KeTaskControlBlock *tcb)
{
    return tcb->blockReason;
}

struct KeTaskControlBlock* KeGetCurrentTask(void)
{
    return currentTask;
}

void KeTaskYield(void)
{
    if(HalGetProcessorPriority() > HAL_PRIORITY_LEVEL_PASSIVE)
        KePanicEx(PRIORITY_LEVEL_TOO_HIGH, HalGetProcessorPriority(), HAL_PRIORITY_LEVEL_PASSIVE, 0, 0);
    KeSchedule();
    HalPerformTaskSwitch();
}

void KePerformPreemptedTaskSwitch(void)
{
    PRIO prio = KeAcquireSpinlock(&dpcTaskSwitchFlagMutex);
    if(true == dpcTaskSwitchPending)
    {
        dpcTaskSwitchPending = false;
        KeReleaseSpinlock(&dpcTaskSwitchFlagMutex, prio);
        HalPerformTaskSwitch();
        return;
    }
    KeReleaseSpinlock(&dpcTaskSwitchFlagMutex, prio);
}