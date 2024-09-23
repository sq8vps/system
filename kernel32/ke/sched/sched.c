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

#define KE_SCHEDULER_TIME_SLICE 100000 //microseconds

struct KeSchedulerQueue
{
    struct KeTaskControlBlock *volatile head;
    KeSpinlock spinlock;
};

#ifndef SMP
struct KeTaskControlBlock *KeCurrentTask[1] = {NULL}; //current task TCB
void *KeCurrentCpuState[1] = {NULL};
struct KeTaskControlBlock *KeNextTask[1] = {NULL}; //next task TCB planned to be switched after DPC processing
void *KeNextCpuState[1] = {NULL};
struct KeTaskControlBlock *KeLastTask[1] = {NULL};
volatile bool KeTaskSwitchPending = false;
#else
struct KeTaskControlBlock *volatile KeCurrentTask[MAX_CPU_COUNT] = {[0 ... MAX_CPU_COUNT - 1] = NULL}; //current task TCB
void *volatile KeCurrentCpuState[MAX_CPU_COUNT] = {[0 ... MAX_CPU_COUNT - 1] = NULL};
struct KeTaskControlBlock *volatile KeNextTask[MAX_CPU_COUNT] = {[0 ... MAX_CPU_COUNT - 1] = NULL}; //next task TCB planned to be switched after DPC processing
void *volatile KeNextCpuState[MAX_CPU_COUNT] = {[0 ... MAX_CPU_COUNT - 1] = NULL};
struct KeTaskControlBlock *volatile KeLastTask[MAX_CPU_COUNT] = {[0 ... MAX_CPU_COUNT - 1] = NULL};
volatile bool KeTaskSwitchPending[MAX_CPU_COUNT] = {[0 ... MAX_CPU_COUNT - 1] = false};
#endif

static struct KeSchedulerQueue KeReadyToRun[PRIORITY_LOWEST + 1][TCB_MINOR_PRIORITY_LIMIT + 1]
    = {[0 ... PRIORITY_LOWEST] = {[0 ... TCB_MINOR_PRIORITY_LIMIT] = {.head = NULL, .spinlock = KeSpinlockInitializer}}}; //next ready to run task
static struct KeSchedulerQueue KeTerminated = {.head = NULL, .spinlock = KeSpinlockInitializer}; //next task to be terminated

static bool KeSchedulerStarted = false;

static void KeSchedule(uint16_t cpu);

//this worker runs always at the DPC level
static void KeSchedulerWorker(void *context)
{
#ifndef SMP
    if(false == KeTaskSwitchPending)
    {
        KeSchedule(0);
        KeTaskSwitchPending = true;
    }
#else
    uint16_t cpu = (uintptr_t)context;
    if(false == KeTaskSwitchPending[cpu])
    {
        KeSchedule(cpu);
        KeTaskSwitchPending[cpu] = true;
    }
#endif
}

STATUS KeSchedulerISR(void *context)
{
#ifndef SMP
    KeRegisterDpc(KE_DPC_PRIORITY_NORMAL, KeSchedulerWorker, NULL);
#else
    uintptr_t cpu = HalGetCurrentCpu();
    KeRegisterDpc(KE_DPC_PRIORITY_NORMAL, KeSchedulerWorker, (void*)cpu);
#endif
    HalStartSystemTimer(KE_SCHEDULER_TIME_SLICE);
    return OK;
}

/**
 * @brief Detach task from current queue
 * @param *tcb TCB pointer
 * @param skipLock True to skip acquiring and releasing spinlock
*/
static void KeDetachTaskFromQueue(struct KeTaskControlBlock *tcb, bool skipLock)
{
    PRIO prio;
    if(NULL == tcb->queue)
    {
        return;
    }
    if(!skipLock)
        prio = KeAcquireSpinlock(&(tcb->queue->spinlock));
    
    if(tcb != tcb->queue->head)
    {
        tcb->next->previous = tcb->previous;
        tcb->previous->next = tcb->next;
        tcb->next = NULL;
        tcb->previous = NULL;
    }
    else
    {
        if(tcb->next == tcb)
        {
            tcb->next = NULL;
            tcb->previous = NULL;
            tcb->queue->head = NULL;
        }
        else
        {
            tcb->next->previous = tcb->previous;
            tcb->previous->next = tcb->next;
            tcb->queue->head = tcb->next;
            tcb->next = NULL;
            tcb->previous = NULL;
        }
    }

    tcb->queue = NULL;

    if(!skipLock)
    {
        KeReleaseSpinlock(&(tcb->queue->spinlock), prio);
    }
}

/**
 * @brief (Re)Attach task to given queue
 * @param *tcb TCB pointer
 * @param *queue Target queue
 * @param skipLock True to skip acquiring and releasing spinlock
*/
static void KeAttachTaskToQueue(struct KeTaskControlBlock *tcb, struct KeSchedulerQueue *queue, bool skipLock)
{
    PRIO prio;
    if(!skipLock)
    {
        prio = KeAcquireSpinlock(&(queue->spinlock));
    }

    if(NULL == queue->head)
    {
        tcb->next = tcb;
        tcb->previous = tcb;
        tcb->queue = queue;
        queue->head = tcb;
    }
    else
    {
        tcb->next = queue->head;
        tcb->previous = queue->head->previous;
        queue->head->previous->next = tcb;
        queue->head->previous = tcb;
        tcb->queue = queue;
    }

    if(!skipLock)
    {
        KeReleaseSpinlock(&(queue->spinlock), prio);
    }
}

void KeAttachLastTask(uint16_t cpu)
{
    if(NULL != KeLastTask[cpu])
    {
        PRIO prio = ObLockObject(KeLastTask[cpu]);
        switch(KeLastTask[cpu]->requestedState)
        {
            case TASK_READY_TO_RUN:
            case TASK_RUNNING:
                KeLastTask[cpu]->state = TASK_READY_TO_RUN;
                KeAttachTaskToQueue(KeLastTask[cpu], &KeReadyToRun[KeLastTask[cpu]->majorPriority][KeLastTask[cpu]->minorPriority], false);
                break;
            case TASK_TERMINATED:
            case TASK_UNINITIALIZED:
                KeLastTask[cpu]->state = TASK_TERMINATED;
                KeAttachTaskToQueue(KeLastTask[cpu], &KeTerminated, false);
                break;
            case TASK_WAITING:
                //the task should be already detached by KeBlockTask()
                KeLastTask[cpu]->state = TASK_WAITING;
                break;
        }
        ObUnlockObject(KeLastTask[cpu], prio);
        KeLastTask[cpu] = NULL;
    }
}

static void KeSchedule(uint16_t cpu)
{
    KeRefreshSleepingTasks();
    KeTimedExclusionRefresh();

    for(uint16_t major = 0; major < (PRIORITY_LOWEST + 1); major++)
    {
        for(uint16_t minor = 0; minor < (TCB_MINOR_PRIORITY_LIMIT + 1); minor++)
        {   
            if(NULL != KeCurrentTask[cpu])
            {
                PRIO prio = ObLockObject(KeCurrentTask[cpu]);
#ifdef SMP
                if(HAL_GET_CPU_BIT(&(KeCurrentTask[cpu]->affinity), cpu))
                {
#endif
                    if((KeCurrentTask[cpu]->majorPriority <= major)
                        && (KeCurrentTask[cpu]->minorPriority < minor))
                    {
                        if((KeCurrentTask[cpu]->requestedState == TASK_RUNNING) 
                        || (KeCurrentTask[cpu]->requestedState == TASK_READY_TO_RUN))
                        {
                            KeNextTask[cpu] = NULL;
                            KeCurrentTask[cpu]->state = TASK_RUNNING;
                            KeCurrentTask[cpu]->requestedState = TASK_READY_TO_RUN;
                            HalStartSystemTimer(KE_SCHEDULER_TIME_SLICE);
                            ObUnlockObject(KeCurrentTask[cpu], prio);
                            return;
                        }
                    }
#ifdef SMP
                }
#endif
                ObUnlockObject(KeCurrentTask[cpu], prio);
            }

            PRIO prio = KeAcquireSpinlock(&KeReadyToRun[major][minor].spinlock);
            if(NULL != KeReadyToRun[major][minor].head)
            {
                PRIO taskPrio = ObLockObject(KeReadyToRun[major][minor].head);
#ifdef SMP
                if(!HAL_GET_CPU_BIT(&(KeReadyToRun[major][minor].head->affinity), cpu))
                {
                    ObUnlockObject(KeReadyToRun[major][minor].head, taskPrio);
                    KeReleaseSpinlock(&KeReadyToRun[major][minor].spinlock, prio);
                    continue;
                }
#endif
                //get next task from queue
                KeNextTask[cpu] = KeReadyToRun[major][minor].head;
                KeNextCpuState[cpu] = &(KeReadyToRun[major][minor].head->cpu);
                KeDetachTaskFromQueue(KeNextTask[cpu], true);
                //update state
                KeNextTask[cpu]->state = TASK_RUNNING;
                KeNextTask[cpu]->requestedState = TASK_READY_TO_RUN;
                ObUnlockObject(KeNextTask[cpu], taskPrio);
                KeReleaseSpinlock(&KeReadyToRun[major][minor].spinlock, prio);
                HalStartSystemTimer(KE_SCHEDULER_TIME_SLICE);
                return;
            }
            KeReleaseSpinlock(&KeReadyToRun[major][minor].spinlock, prio);
        }
    }


    if(NULL != KeCurrentTask[cpu])
    {
        PRIO prio = ObLockObject(KeCurrentTask[cpu]);
#ifdef SMP
        if(HAL_GET_CPU_BIT(&(KeCurrentTask[cpu]->affinity), cpu))
        {
#endif
            if((KeCurrentTask[cpu]->requestedState == TASK_RUNNING) 
            || (KeCurrentTask[cpu]->requestedState == TASK_READY_TO_RUN))
            {
                KeNextTask[cpu] = NULL;
                KeCurrentTask[cpu]->state = TASK_RUNNING;
                KeCurrentTask[cpu]->requestedState = TASK_READY_TO_RUN;
                ObUnlockObject(KeCurrentTask[cpu], prio);
                HalStartSystemTimer(KE_SCHEDULER_TIME_SLICE);
                return;
            }
#ifdef SMP
        }
#endif
        ObUnlockObject(KeCurrentTask[cpu], prio);
    }

    //should never reach this point
    KePanicEx(NO_EXECUTABLE_TASK, cpu, 0, 0, 0);
}

NORETURN void KeStartScheduler(void (*continuationTask)(void*), void *continuationContext)
{   
    STATUS ret = OK;
    //create idle task
    if(OK != (ret = KeCreateIdleTask()))
        KePanicEx(BOOT_FAILURE, KE_SCHEDULER_INITIALIZATION_FAILURE, ret, 0, 0);
    
    if(OK != (ret = KeCreateIdleTask()))
        KePanicEx(BOOT_FAILURE, KE_SCHEDULER_INITIALIZATION_FAILURE, ret, 0, 0);
    
    if(NULL != continuationTask)
    {
        struct KeTaskControlBlock *tcb;
        if(OK != (ret = KeCreateProcessRaw("KernelInit", NULL, PL_KERNEL, continuationTask, continuationContext, &tcb)))
            KePanicEx(BOOT_FAILURE, KE_SCHEDULER_INITIALIZATION_FAILURE, ret, 1, 0);
        
        KeChangeTaskMajorPriority(tcb, PRIORITY_NORMAL);
        KeChangeTaskMinorPriority(tcb, TCB_DEFAULT_MINOR_PRIORITY);
        KeEnableTask(tcb);
    }

    if(OK != (ret = ItInstallInterruptHandler(IT_SYSTEM_TIMER_VECTOR, KeSchedulerISR, NULL)))
        KePanicEx(BOOT_FAILURE, KE_SCHEDULER_INITIALIZATION_FAILURE, ret, 2, 0);
    if(OK != (ret = ItSetInterruptHandlerEnable(IT_SYSTEM_TIMER_VECTOR, KeSchedulerISR, true)))
        KePanicEx(BOOT_FAILURE, KE_SCHEDULER_INITIALIZATION_FAILURE, ret, 3, 0);
        
    HalInitializeScheduler();

    __atomic_store_n(&KeSchedulerStarted, true, __ATOMIC_SEQ_CST);

    HalConfigureSystemTimer(IT_SYSTEM_TIMER_VECTOR);
    HalStartSystemTimer(KE_SCHEDULER_TIME_SLICE);

    while(1)
        KeTaskYield();
}


STATUS KeChangeTaskMajorPriority(struct KeTaskControlBlock *tcb, enum KeTaskMajorPriority priority)
{
    if(NULL == tcb)
        return NULL_POINTER_GIVEN;

    PRIO prio = ObLockObject(tcb);
    tcb->majorPriority = priority;
    ObUnlockObject(tcb, prio);

    return OK;
}

STATUS KeChangeTaskMinorPriority(struct KeTaskControlBlock *tcb, uint8_t priority)
{
    if(NULL == tcb)
        return NULL_POINTER_GIVEN;

    PRIO prio = ObLockObject(tcb);
    if(priority > TCB_MINOR_PRIORITY_LIMIT)
        priority = TCB_MINOR_PRIORITY_LIMIT;

    tcb->minorPriority = priority;
    ObUnlockObject(tcb, prio);

    return OK;
}

STATUS KeEnableTask(struct KeTaskControlBlock *tcb)
{
    if(NULL == tcb)
        return NULL_POINTER_GIVEN;

    PRIO prio = ObLockObject(tcb);

    if(TASK_UNINITIALIZED == tcb->state)
    {
        tcb->requestedState = TASK_READY_TO_RUN;
        //PRIO p = KeAcquireSpinlock(&lock);
        KeAttachTaskToQueue(tcb, &KeReadyToRun[tcb->majorPriority][tcb->minorPriority], false);
        //KeReleaseSpinlock(&lock, p);
    }
    
    ObUnlockObject(tcb, prio);

    return OK;
}

void KeBlockTask(struct KeTaskControlBlock *tcb, enum KeTaskBlockReason reason)
{
    PRIO prio = ObLockObject(tcb);
    if(tcb->flags & KE_TASK_FLAG_IDLE)
        KePanic(UNEXPECTED_FAULT);
    tcb->requestedState = TASK_WAITING;
    tcb->blockReason = reason;
    KeDetachTaskFromQueue(tcb, false);
    ObUnlockObject(tcb, prio);
}

void KeUnblockTask(struct KeTaskControlBlock *tcb)
{
    PRIO prio = ObLockObject(tcb);
    tcb->blockReason = TASK_BLOCK_NOT_BLOCKED;
    //if the task is running on an CPU, it must stay detached from any queue
    //this prevents other CPUs in a SMP system to execute the same task simultaneously
    //in UP systems the problem is almost the same - the CPU might be currently executing the task,
    //so it must stay detached
    //just check if task is waiting
    if(tcb->state == TASK_WAITING)
        KeAttachTaskToQueue(tcb, &KeReadyToRun[tcb->majorPriority][tcb->minorPriority], false);
    
    if(TASK_RUNNING == tcb->state)
        tcb->requestedState = TASK_READY_TO_RUN;
    else
        tcb->state = TASK_READY_TO_RUN;
    ObUnlockObject(tcb, prio);
}

enum KeTaskBlockReason KeGetTaskBlockState(struct KeTaskControlBlock *tcb)
{
    return tcb->blockReason;
}

struct KeTaskControlBlock* KeGetCurrentTask(void)
{
#ifndef SMP
    return KeCurrentTask[0];
#else
    return KeCurrentTask[HalGetCurrentCpu()];
#endif
}

struct KeTaskControlBlock* KeGetCurrentTaskParent(void)
{
    struct KeTaskControlBlock *t = KeGetCurrentTask();
    if((NULL != t) && (NULL != t->parent))
        return t->parent;
    else
        return t;
}

void KeTaskYield(void)
{
    if(HalGetProcessorPriority() > HAL_PRIORITY_LEVEL_PASSIVE)
        KePanicEx(PRIORITY_LEVEL_TOO_HIGH, HalGetProcessorPriority(), HAL_PRIORITY_LEVEL_PASSIVE, 0, 0);
    
    PRIO prio = HalRaisePriorityLevel(HAL_PRIORITY_LEVEL_DPC);
    //raise priority level to DPC to ensure that there will be no system timer IRQ
    //this is required by the scheduler
#ifndef SMP
    KeSchedule(0);
    KeTaskSwitchPending = true;
#else
    uint16_t cpu = HalGetCurrentCpu();
    if(false == KeTaskSwitchPending[cpu])
    {
        KeSchedule(cpu);
        KeTaskSwitchPending[cpu] = true;
    }
    else
        KePanic(UNEXPECTED_FAULT);
    
    
#endif
    HalLowerPriorityLevel(prio);
    HalPerformTaskSwitch();
}

void KeJoinScheduler(void)
{
    while(false == __atomic_load_n(&KeSchedulerStarted, __ATOMIC_SEQ_CST))
        TIGHT_LOOP_HINT();
    if(OK != KeCreateIdleTask())
    {
        while(1)
            ;
    }
    if(OK != KeCreateIdleTask())
    {
        while(1)
            ;
    }
    HalConfigureSystemTimer(IT_SYSTEM_TIMER_VECTOR);
    HalStartSystemTimer(KE_SCHEDULER_TIME_SLICE);
}

#if false != 0
#error False is not zero!
#endif
#if true != 1
#error True is not one!
#endif