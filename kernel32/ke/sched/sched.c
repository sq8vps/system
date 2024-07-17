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

#define KE_SCHEDULER_TIME_SLICE 10000 //microseconds

static bool dpcTaskSwitchPending = false;
static KeSpinlock dpcTaskSwitchFlagMutex = KeSpinlockInitializer;

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
    void *KeCurrentCpuState;
    struct KeTaskControlBlock *nextTask = NULL; //next task TCB planned to be switched after DPC processing
    void *KeNextCpuState = NULL;
    static struct KeTaskControlBlock *readyToRun[PRIORITY_LOWEST + 1][TCB_MINOR_PRIORITY_LIMIT + 1]; //next ready to run task
    static struct KeTaskControlBlock *terminated; //next task to be terminated
#endif

static void KeSchedule(void);

//this worker runs always at the DPC level
static void KeSchedulerWorker(void *context)
{
    KeSchedule();
    KeAcquireSpinlock(&dpcTaskSwitchFlagMutex);
    dpcTaskSwitchPending = true;
    KeReleaseSpinlock(&dpcTaskSwitchFlagMutex);
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
    KeAcquireSpinlock(&queueMutex);
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
    KeReleaseSpinlock(&queueMutex);
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
    KeAcquireSpinlock(&queueMutex);
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
    KeReleaseSpinlock(&queueMutex);
}

static KeSpinlock schedulerMutex = KeSpinlockInitializer;

static void KeSchedule(void)
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
                //get next task from queue
                nextTask = readyToRun[major][minor];
                KeNextCpuState = &(readyToRun[major][minor]->cpu);
                //update state
                nextTask->state = TASK_RUNNING;
                KeReleaseSpinlock(&schedulerMutex);
                HalStartSystemTimer(KE_SCHEDULER_TIME_SLICE);
                return;
            }
        }
    }
    //should never reach this point
    KePanic(NO_EXECUTABLE_TASK);
}

void KeSchedulerStart(void)
{
    CmMemset(readyToRun, 0, sizeof(readyToRun));
    terminated = NULL;
    
    STATUS ret = OK;
    //create idle task
    if(OK != (ret = KeCreateIdleTask()))
        KePanicEx(BOOT_FAILURE, KE_SCHEDULER_INITIALIZATION_FAILURE, ret, 0, 0);

    if(OK != (ret = ItInstallInterruptHandler(IT_SYSTEM_TIMER_VECTOR, KeSchedulerISR, NULL)))
        KePanicEx(BOOT_FAILURE, KE_SCHEDULER_INITIALIZATION_FAILURE, ret, 0, 0);
    if(OK != (ret = ItSetInterruptHandlerEnable(IT_SYSTEM_TIMER_VECTOR, KeSchedulerISR, true)))
        KePanicEx(BOOT_FAILURE, KE_SCHEDULER_INITIALIZATION_FAILURE, ret, 0, 0);
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
    KeCurrentCpuState = &(tcb->cpu);

    HalConfigureSystemTimer(IT_SYSTEM_TIMER_VECTOR);
    HalStartSystemTimer(KE_SCHEDULER_TIME_SLICE);

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
    KePerformTaskSwitch();
}

void KePerformPreemptedTaskSwitch(void)
{
    KeAcquireSpinlock(&dpcTaskSwitchFlagMutex);
    if(true == dpcTaskSwitchPending)
    {
        dpcTaskSwitchPending = false;
        KeReleaseSpinlock(&dpcTaskSwitchFlagMutex);
        KePerformTaskSwitch();
        return;
    }
    KeReleaseSpinlock(&dpcTaskSwitchFlagMutex);
}