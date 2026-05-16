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

/**
 * @brief System tick to be used when the time slice is indefinite (in nanoseconds)
 */
#define KE_DEFAULT_SYSTEM_TICK MS_TO_NS(1)

#ifndef SMP
struct
{
    struct KeTaskControlBlock *volatile task;
    void *volatile cpuState;
    uint64_t slice;
}
KeCurrentTask[1] = {0}, 
KeNextTask[1] = {0};

static KeSpinlock KeSchedulingLock = KeSpinlockInitializer;
volatile bool KeTaskSwitchPending = false;
volatile bool KeTaskSwitchInProgress = false;
#else
struct
{
    struct KeTaskControlBlock *volatile task;
    void *volatile cpuState;
    uint32_t preemptible;
    uint64_t slice;
    uint32_t unused[3];
}
KeCurrentTask[MAX_CPU_COUNT] = {0}, 
KeNextTask[MAX_CPU_COUNT] = {0};

//static KeSpinlock KeSchedulingLock[MAX_CPU_COUNT] = {KeSpinlockInitializer};
static KeSpinlock KeSchedulingLock = KeSpinlockInitializer;
volatile bool KeTaskSwitchPending[MAX_CPU_COUNT] = {false};
volatile bool KeTaskSwitchInProgress[MAX_CPU_COUNT] = {false};

#endif

static struct KeTaskControlBlock *KeCleanupTask = nullptr;
static struct
{
    struct KeTaskControlBlock *head;
    struct KeTaskControlBlock *tail;
    KeSpinlock lock;
} 
KeFinishedTasks = {.head = nullptr, .tail = nullptr, .lock = KeSpinlockInitializer};

static uint32_t KeJoinedCpus = 0; 

/**
 * @brief Perform task switch immediately if a new task is available
*/
INTERNAL void HalPerformTaskSwitch(void);

static void KeSchedule(uint32_t cpu);
static void KeTaskCleanupWorker(void *context);

//this worker runs always at the DPC level
static void KeSchedulerWorker(void *context)
{
#ifndef SMP
    UNUSED(context);
    if(KeCurrentTask[0].preemptible)
    {
        KeTaskSwitchPending = true;
    }
#else
    uint32_t cpu = (uint32_t)context;
    if(KeCurrentTask[cpu].preemptible)
    {
        KeTaskSwitchPending[cpu] = true;
    }
#endif
}

STATUS KeSchedulerISR(void *context)
{
    UNUSED(context);
    HalUpdateSystemTimerOnInterrupt();
#ifndef SMP
    KeRegisterDpc(KE_DPC_PRIORITY_LOW, KeSchedulerWorker, NULL, true);
#else
    uint32_t cpu = HalGetCurrentCpu();
    KeRegisterDpc(KE_DPC_PRIORITY_LOW, KeSchedulerWorker, (void*)cpu, true);
#endif
    return OK;
}

FASTCALL
void KeUpdateTaskScheduleTime(struct KeTaskControlBlock *tcb)
{
    tcb->scheduling.lastScheduled = HalGetTimestamp();
}

static inline void KeQueueTask(struct KeTaskControlBlock *tcb)
{
    switch(tcb->scheduling.policy)
    {
        case KE_SCHED_FCFS:
            KeFcfsQueueTask(tcb);
            break;
        case KE_SCHED_RR:
            KeRrQueueTask(tcb);
            break;
        case KE_SCHED_CFS:
        case KE_SCHED_IDLE:
            KeCfsQueueTask(tcb);
            break;
    }
}

FASTCALL
void KeHandleLastTask(struct KeTaskControlBlock *tcb)
{
    if(unlikely(nullptr == tcb))
        return;
    PRIO prio = KeAcquireSpinlock(&tcb->scheduling.lock);
    if(TASK_RUNNING == tcb->scheduling.state)
        KeQueueTask(tcb);
    else if(TASK_RUNNING_BLOCK == tcb->scheduling.state)
        tcb->scheduling.state = TASK_BLOCKED;
    KeReleaseSpinlock(&tcb->scheduling.lock, prio);
}

static void KeSchedule(uint32_t cpu)
{
    KeRefreshSleepingTasks();
    KeTimedExclusionRefresh();

    struct KeTaskControlBlock *current = KeCurrentTask[cpu].task;
    struct KeTaskControlBlock *next = nullptr;

    if(likely(nullptr != current))
    {
        PRIO prio = KeAcquireSpinlock(&current->scheduling.lock);
        current->scheduling.lastRuntime = HalGetTimestamp() - current->scheduling.lastScheduled;
        bool wakeCleanup = false;
        
        if(TASK_FINISHED == current->scheduling.state)
        {
            PRIO prio = KeAcquireSpinlock(&KeFinishedTasks.lock);
            if(nullptr != KeFinishedTasks.tail)
            {
                KeFinishedTasks.tail->scheduling.next = current;
                KeFinishedTasks.tail = current;
                current->scheduling.next = nullptr;
            }
            else
            {
                KeFinishedTasks.head = current;
                KeFinishedTasks.tail = current;
                current->scheduling.next = nullptr;
            }
            KeReleaseSpinlock(&KeFinishedTasks.lock, prio);
            wakeCleanup = true;
        }
        KeReleaseSpinlock(&current->scheduling.lock, prio);

        if(wakeCleanup)
            KeWakeUpTask(KeCleanupTask);
    }

    next = KeFcfsGetNextTask(cpu, current, &KeNextTask[cpu].slice);
    if(nullptr == next)
    {
        next = KeRrGetNextTask(cpu, current, &KeNextTask[cpu].slice);
        if(nullptr == next)
        {
            next = KeCfsGetNextTask(cpu, current, &KeNextTask[cpu].slice);
            if(unlikely(nullptr == next))
                KePanicEx(NO_EXECUTABLE_TASK, cpu, 0, 0, 0);
        }
    }

    next->scheduling.state = TASK_RUNNING;
    KeNextTask[cpu].task = next;
    KeNextTask[cpu].cpuState = &next->data;
    if(0 != KeNextTask[cpu].slice)
        KeNextTask[cpu].preemptible = 1;
    else
        KeNextTask[cpu].preemptible = 0;
}

[[noreturn]] void KeStartScheduler(void (*continuationTask)(void*), void *continuationContext)
{   
    STATUS ret = OK;
    //create idle task
    if(OK != (ret = KeCreateIdleTask()))
        KePanicEx(BOOT_FAILURE, 1, ret, 0, 0);
    
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
    HalStartSystemTimer(KE_DEFAULT_SYSTEM_TICK);

    KeTaskYield();
    
    while(1)
        HALT();
}

STATUS KeEnableTask(struct KeTaskControlBlock *tcb)
{
    if(NULL == tcb)
        return BAD_PARAMETER;

    if(TASK_UNINITIALIZED == tcb->scheduling.state)
        KeQueueTask(tcb);

    return OK;
}

[[noreturn]] void KeFinishCurrentTask(int result)
{
    UNUSED(result);
    //TODO: handle return code
    struct KeTaskControlBlock *tcb = KeGetCurrentTask();
    ATOMIC_STORE(&tcb->scheduling.state, TASK_FINISHED, ATOMIC_RELAXED);
    KeTaskYield();

    while(1)
        ;
}

void KeBlockTask(enum KeTaskBlockReason reason)
{
    if(unlikely(TASK_BLOCK_SLEEP == reason))
        return;

    struct KeTaskControlBlock *tcb = KeGetCurrentTask();
    //A task can only block itself, so it must be running at that time. It's deattached from all queues then.
    PRIO prio = KeAcquireSpinlock(&(tcb->scheduling.lock));
    if(unlikely(tcb->flags & KE_TASK_FLAG_IDLE))
        KePanic(UNEXPECTED_FAULT);
    tcb->scheduling.state = TASK_RUNNING_BLOCK;
    tcb->scheduling.block.reason = reason;
    KeReleaseSpinlock(&(tcb->scheduling.lock), prio);
}

void KeUnblockTask(struct KeTaskControlBlock *tcb)
{
    //there might be a scenario when the task requested a block but did not yield yet, and is still running
    //this is the TASK_RUNNING_BLOCK case
    //in such a case we can't queue the task, as it may result in multiple CPUs executing the same task
    PRIO prio = KeAcquireSpinlock(&(tcb->scheduling.lock));
    tcb->scheduling.block.reason = TASK_BLOCK_NOT_BLOCKED;
    if(TASK_RUNNING_BLOCK == tcb->scheduling.state)
        tcb->scheduling.state = TASK_RUNNING;
    else if(TASK_BLOCKED == tcb->scheduling.state)
        KeQueueTask(tcb);
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
        tcb->scheduling.state = TASK_RUNNING_BLOCK;
        tcb->scheduling.block.reason = TASK_BLOCK_SLEEP;
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
        tcb->scheduling.block.reason = TASK_BLOCK_NOT_BLOCKED;
        tcb->scheduling.state = TASK_READY_TO_RUN;
        KeQueueTask(tcb);
    }

    KeReleaseSpinlock(&(tcb->scheduling.lock), prio);   
}

struct KeTaskControlBlock* KeGetCurrentTask(void)
{
#ifndef SMP
    return KeCurrentTask[0].task;
#else
    struct KeTaskControlBlock *task;
    if(HalGetProcessorPriority() < HAL_PRIORITY_LEVEL_DPC)
    {
        HalRaisePriorityLevel(HAL_PRIORITY_LEVEL_DPC);
        task = KeCurrentTask[HalGetCurrentCpu()].task;
        HalLowerPriorityLevel(HAL_PRIORITY_LEVEL_PASSIVE);
    }
    else
    {
        task = KeCurrentTask[HalGetCurrentCpu()].task;
    }
    return task;
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
        KePanicEx(PRIORITY_LEVEL_TOO_HIGH, HalGetProcessorPriority(), HAL_PRIORITY_LEVEL_PASSIVE, HalGetCurrentCpu(), 0);

#ifndef SMP
    ATOMIC_STORE(&KeTaskSwitchPending, true, ATOMIC_SEQ_CST);
#else
    HalRaisePriorityLevel(HAL_PRIORITY_LEVEL_DPC);
    ATOMIC_STORE(&KeTaskSwitchPending[HalGetCurrentCpu()], true, ATOMIC_SEQ_CST);
    HalLowerPriorityLevel(HAL_PRIORITY_LEVEL_PASSIVE);
#endif
    KePerformTaskSwitch();
}

void KePerformTaskSwitch(void)
{
    HalRaisePriorityLevel(HAL_PRIORITY_LEVEL_DPC);
    uint32_t cpu = HalGetCurrentCpu();
#ifndef SMP
    if(!KeTaskSwitchPending || KeTaskSwitchInProgress)
#else
    if(!KeTaskSwitchPending[cpu] || KeTaskSwitchInProgress[cpu])
#endif
    {
        HalLowerPriorityLevel(HAL_PRIORITY_LEVEL_PASSIVE);
        return;
    }

#ifndef SMP
    KeAcquireDpcLevelSpinlock(&KeSchedulingLock);
    barrier();
    KeTaskSwitchInProgress = true;
    barrier();
    KeTaskSwitchPending = false;
#else
    KeAcquireDpcLevelSpinlock(&KeSchedulingLock);
    barrier();
    KeTaskSwitchInProgress[cpu] = true;
    barrier();
    KeTaskSwitchPending[cpu] = false;
#endif
    KeSchedule(cpu);
    HalStartSystemTimer(KeNextTask[cpu].preemptible ? KeNextTask[cpu].slice : KE_DEFAULT_SYSTEM_TICK);
    HalPerformTaskSwitch();

    barrier();
    cpu = HalGetCurrentCpu();
#ifndef SMP
    barrier();
    KeTaskSwitchInProgress = false;
    KeReleaseSpinlock(&KeSchedulingLock, HAL_PRIORITY_LEVEL_PASSIVE);
#else
    barrier();
    KeTaskSwitchInProgress[cpu] = false;
    barrier();
    KeReleaseSpinlock(&KeSchedulingLock, HAL_PRIORITY_LEVEL_PASSIVE);
#endif
    //no need to lower the priority again, it is done in the spinlock release above
}

void KeReleaseInitialSchedulingLock(void)
{
#ifndef SMP
    KeTaskSwitchInProgress = false;
#else
    KeTaskSwitchInProgress[HalGetCurrentCpu()] = false;
#endif
    KeReleaseSpinlock(&KeSchedulingLock, HAL_PRIORITY_LEVEL_PASSIVE);
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
    HalConfigureSystemTimer(IT_SYSTEM_TIMER_VECTOR);
    HalStartSystemTimer(KE_DEFAULT_SYSTEM_TICK);

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
        PRIO prio = KeAcquireSpinlock(&(KeFinishedTasks.lock));
        struct KeTaskControlBlock *t = KeFinishedTasks.head;

        if(NULL != t)
        {
            KeFinishedTasks.head = t->scheduling.next;
            if(nullptr == KeFinishedTasks.head)
                KeFinishedTasks.tail = nullptr;
            barrier();
            KeReleaseSpinlock(&(KeFinishedTasks.lock), prio);

            KeDestroyTask(t);
        }
        else
        {
            KeReleaseSpinlock(&(KeFinishedTasks.lock), prio);
        }
        KeWaitForWakeUp();
    }
}