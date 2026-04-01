#include "sched.h"
#include <stdbool.h>
#include "ke/core/panic.h"
#include "idle.h"
#include "rtl/string.h"
#include "hal/task.h"
#include "hal/interrupt.h"
#include "ke/core/mutex.h"
#include "it/it.h"
#include "sleep.h"
#include "ke/core/dpc.h"
#include "hal/arch.h"
#include "hal/time.h"
#include "ex/worker.h"
#include "hal/hal.h"
#include "fcfs.h"
#include "rr.h"
#include "cfs.h"

#ifndef SMP
struct
{
    struct KeTaskControlBlock *volatile task;
    void *volatile cpuState;
    uint64_t slice;
}
KeCurrentTask[1] = {0}, 
KeNextTask[1] = {0};

volatile bool KeTaskSwitchPending = false;
volatile bool KeTaskSwitchInProgress = false;
#else
struct
{
    struct KeTaskControlBlock *volatile task;
    void *volatile cpuState;
    uint64_t slice;
}
KeCurrentTask[MAX_CPU_COUNT] = {0}, 
KeNextTask[MAX_CPU_COUNT] = {0};

volatile bool KeTaskSwitchPending[MAX_CPU_COUNT] = {0};
volatile bool KeTaskSwitchInProgress[MAX_CPU_COUNT] = {0};
#endif

static struct KeTaskControlBlock *KeCleanupTask = NULL;

static uint32_t KeJoinedCpus = 0; 

static void KeSchedule(uint32_t cpu);
static void KeTaskCleanupWorker(void *context);

//this worker runs always at the DPC level
static void KeSchedulerWorker(void *context)
{
#ifndef SMP
    UNUSED(context);
    if((false == KeTaskSwitchPending) && (false == KeTaskSwitchInProgress))
    {
        KeSchedule(0);
        KeTaskSwitchPending = true;
    }
#else
    uint32_t cpu = (uint32_t)context;
    if((false == KeTaskSwitchPending[cpu]) && (false == KeTaskSwitchInProgress[cpu]))
    {
        KeSchedule(cpu);
        KeTaskSwitchPending[cpu] = true;
    }
#endif
}

STATUS KeSchedulerISR(void *context)
{
    UNUSED(context);
#ifndef SMP
    KeRegisterDpc(KE_DPC_PRIORITY_NORMAL, KeSchedulerWorker, NULL);
#else
    uint32_t cpu = HalGetCurrentCpu();
    KeRegisterDpc(KE_DPC_PRIORITY_NORMAL, KeSchedulerWorker, (void*)cpu);
#endif
    return OK;
}

static void KeSchedule(uint32_t cpu)
{
    KeRefreshSleepingTasks();
    KeTimedExclusionRefresh();

    struct KeTaskControlBlock *current = KeCurrentTask[cpu].task;
    struct KeTaskControlBlock *next = nullptr;

    if(TASK_RUNNING == current->scheduling.state)
    {
        switch(current->scheduling.policy)
        {
            case KE_SCHED_FCFS:
                KeFcfsQueueTask(current);
                break;
            case KE_SCHED_RR:
                KeRrQueueTask(current);
                break;
            case KE_SCHED_CFS:
            case KE_SCHED_IDLE:
                KeCfsQueueTask(current);
                break;
        }
    }

    //should never reach this point
    KePanicEx(NO_EXECUTABLE_TASK, cpu, 0, 0, 0);
}

[[noreturn]] void KeStartScheduler(void (*continuationTask)(void*), void *continuationContext)
{   
    STATUS ret = OK;
    //create idle task
    if(OK != (ret = KeCreateIdleTask()))
        KePanicEx(BOOT_FAILURE, 1, ret, 0, 0);
    
    // if(OK != (ret = KeCreateIdleTask()))
    //     KePanicEx(BOOT_FAILURE, 1, ret, 0, 0);
    
    if(NULL != continuationTask)
    {
        struct KeTaskControlBlock *tcb;
        if(OK != (ret = KeCreateKernelProcess(0, continuationTask, continuationContext, NULL, &tcb)))
            KePanicEx(BOOT_FAILURE, 1, ret, 1, 0);
        
        KeEnableTask(tcb);
    }

    ret = ExCreateKernelWorker(KeTaskCleanupWorker, NULL, &KeCleanupTask);
    if(OK != ret)
        KePanicEx(BOOT_FAILURE, 1, ret, 2, 0);

    if(OK != (ret = ItInstallInterruptHandler(IT_SYSTEM_TIMER_VECTOR, KeSchedulerISR, NULL)))
        KePanicEx(BOOT_FAILURE, 1, ret, 3, 0);
    if(OK != (ret = ItSetInterruptHandlerEnable(IT_SYSTEM_TIMER_VECTOR, KeSchedulerISR, true)))
        KePanicEx(BOOT_FAILURE, 1, ret, 4, 0);
        
    HalInitializeScheduler();

    ATOMIC_ADD_FETCH(&KeJoinedCpus, 1, ATOMIC_SEQ_CST);

    HalConfigureSystemTimer(IT_SYSTEM_TIMER_VECTOR);
    HalStartSystemTimer(10000);

    KeTaskYield();
    
    while(1)
        HALT();
}

STATUS KeEnableTask(struct KeTaskControlBlock *tcb)
{
    if(NULL == tcb)
        return BAD_PARAMETER;

    PRIO prio = KeAcquireSpinlock(&(tcb->scheduling.lock));

    if(TASK_UNINITIALIZED == tcb->scheduling.state)
    {
        tcb->scheduling.requestedState = TASK_READY_TO_RUN;
        KeAttachTaskToQueue(tcb, &KeReadyToRun[tcb->scheduling.majorPriority][tcb->scheduling.minorPriority], false);
    }
    
    KeReleaseSpinlock(&(tcb->scheduling.lock), prio);

    return OK;
}

[[noreturn]] void KeFinishCurrentTask(int result)
{
    UNUSED(result);
    //TODO: handle return code
    struct KeTaskControlBlock *tcb = KeGetCurrentTask();
    tcb->scheduling.requestedState = TASK_FINISHED;
    KeTaskYield();

    while(1)
        ;
}

void KeBlockTask(struct KeTaskControlBlock *tcb, enum KeTaskBlockReason reason)
{
    if(TASK_BLOCK_SLEEP == reason)
        return;

    PRIO prio = KeAcquireSpinlock(&(tcb->scheduling.lock));
    if(unlikely(tcb->flags & KE_TASK_FLAG_IDLE))
        KePanic(UNEXPECTED_FAULT);
    tcb->scheduling.requestedState = TASK_BLOCKED;
    tcb->scheduling.block.reason = reason;
    KeDetachTaskFromQueue(tcb, false);
    KeReleaseSpinlock(&(tcb->scheduling.lock), prio);
}

void KeUnblockTask(struct KeTaskControlBlock *tcb)
{
    PRIO prio = KeAcquireSpinlock(&(tcb->scheduling.lock));

    if(TASK_BLOCK_SLEEP != tcb->scheduling.block.reason)
    {
        tcb->scheduling.block.reason = TASK_BLOCK_NOT_BLOCKED;
        //if the task is running on an CPU, it must stay detached from any queue
        //this prevents other CPUs in a SMP system to execute the same task simultaneously
        //in UP systems the problem is almost the same - the CPU might be currently executing the task,
        //so it must stay detached
        //just check if task is waiting
        if(tcb->scheduling.state == TASK_BLOCKED)
            KeAttachTaskToQueue(tcb, &KeReadyToRun[tcb->scheduling.majorPriority][tcb->scheduling.minorPriority], false);
        
        if(TASK_RUNNING == tcb->scheduling.state)
            tcb->scheduling.requestedState = TASK_READY_TO_RUN;
        else
            tcb->scheduling.state = TASK_READY_TO_RUN;
    }
    KeReleaseSpinlock(&(tcb->scheduling.lock), prio);
}

void KeWaitForWakeUp(void)
{
    struct KeTaskControlBlock *tcb = KeGetCurrentTask();
    PRIO prio = KeAcquireSpinlock(&(tcb->scheduling.lock));
    if(tcb->scheduling.notified)
    {
        tcb->scheduling.notified = false;
        KeReleaseSpinlock(&(tcb->scheduling.lock), prio);
    }
    else
    {
        tcb->scheduling.state = TASK_BLOCKED;
        tcb->scheduling.block.reason = TASK_BLOCK_SLEEP;
        KeDetachTaskFromQueue(tcb, false);
        KeReleaseSpinlock(&(tcb->scheduling.lock), prio);
        KeTaskYield();
    }
}

void KeWakeUpTask(struct KeTaskControlBlock *tcb)
{
    PRIO prio = KeAcquireSpinlock(&(tcb->scheduling.lock));

    tcb->scheduling.notified = true;

    //task might be reattached only when it is waiting for an event
    if((tcb->scheduling.state == TASK_BLOCKED) && (tcb->scheduling.block.reason == TASK_BLOCK_SLEEP))
    {
        KeAttachTaskToQueue(tcb, &KeReadyToRun[tcb->scheduling.majorPriority][tcb->scheduling.minorPriority], false);
        tcb->scheduling.state = TASK_READY_TO_RUN;
        tcb->scheduling.requestedState = TASK_READY_TO_RUN;
        tcb->scheduling.block.reason = TASK_BLOCK_NOT_BLOCKED;
    }

    KeReleaseSpinlock(&(tcb->scheduling.lock), prio);   
}

struct KeTaskControlBlock* KeGetCurrentTask(void)
{
#ifndef SMP
    return KeCurrentTask[0].task;
#else
    return KeCurrentTask[HalGetCurrentCpu()].task;
#endif
}

struct KeProcessControlBlock* KeGetCurrentTaskParent(void)
{
    struct KeTaskControlBlock *t = KeGetCurrentTask();
    if(NULL == t)
        return NULL;

    return t->parent;
}

void KeTaskYield(void)
{
    if(unlikely(HalGetProcessorPriority() > HAL_PRIORITY_LEVEL_PASSIVE))
        KePanicEx(PRIORITY_LEVEL_TOO_HIGH, HalGetProcessorPriority(), HAL_PRIORITY_LEVEL_PASSIVE, 0, 0);
    
    PRIO prio = HalRaisePriorityLevel(HAL_PRIORITY_LEVEL_DPC);
    //raise priority level to DPC to ensure that there will be no system timer IRQ
    //this is required by the scheduler
#ifndef SMP
    KeSchedule(0);
    KeTaskSwitchPending = true;
#else
    uint32_t cpu = HalGetCurrentCpu();
    if(likely((false == KeTaskSwitchPending[cpu]) && (false == KeTaskSwitchInProgress[cpu])))
    {
        KeSchedule(cpu);
        KeTaskSwitchPending[cpu] = true;
    }
    else
        KePanic(UNEXPECTED_FAULT);
    
    
#endif
    HalLowerPriorityLevel(prio);
    barrier();
    HalPerformTaskSwitch();
}

void KeJoinScheduler(void)
{
    while(0 == ATOMIC_LOAD(&KeJoinedCpus, ATOMIC_SEQ_CST))
        TIGHT_LOOP_HINT();
    if(OK != KeCreateIdleTask())
    {
        while(1)
            ;
    }
    // if(OK != KeCreateIdleTask())
    // {
    //     while(1)
    //         ;
    // }
    HalConfigureSystemTimer(IT_SYSTEM_TIMER_VECTOR);
    HalStartSystemTimer(10000);

    ATOMIC_ADD_FETCH(&KeJoinedCpus, 1, ATOMIC_SEQ_CST);
}

void KeWaitForCpusToJoinScheduler(uint32_t cpus)
{
    while(cpus > ATOMIC_LOAD(&KeJoinedCpus, ATOMIC_SEQ_CST))
        TIGHT_LOOP_HINT();
}

static void KeTaskCleanupWorker(void *context)
{
    UNUSED(context);
    while(1)
    {
        // PRIO prio = KeAcquireSpinlock(&(KeFinished.lock));
        // struct KeTaskControlBlock *t = KeFinished.head;

        // if(NULL != t)
        // {
        //     KeDetachTaskFromQueue(t, true);
        //     barrier();
        //     KeReleaseSpinlock(&(KeFinished.lock), prio);

        //     KeDestroyTask(t);
        // }
        // else
        // {
        //     KeReleaseSpinlock(&(KeFinished.lock), prio);
        // }
        KeWaitForWakeUp();
    }
}