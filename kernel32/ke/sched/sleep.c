#include "sleep.h"
#include "sched.h"
#include "hal/time.h"
#include "ke/core/mutex.h"
#include "ke/task/task.h"

static struct KeTaskControlBlock *list = NULL;

static KeSpinlock listLock = KeSpinlockInitializer;

STATUS KePutTaskToSleep(struct KeTaskControlBlock *tcb, uint64_t time)
{
    KeBlockTask(tcb, TASK_BLOCK_TIMED_SLEEP);
    tcb->scheduling.block.timeout.until = HalGetTimestamp() + time;

    PRIO prio = KeAcquireSpinlock(&listLock);
    if(NULL == list)
        list = tcb;
    else
    {
        struct KeTaskControlBlock *s = list;
        //keep list sorted: the earliest deadline is first
        while(NULL != s->scheduling.next)
        {
            if((s->scheduling.block.timeout.until <= tcb->scheduling.block.timeout.until) && (s->scheduling.next->scheduling.block.timeout.until > tcb->scheduling.block.timeout.until))
            {
                tcb->scheduling.next = s->scheduling.next;
                break;
            }
            s = s->scheduling.next;
        }
        s->scheduling.next = tcb;
    }
    KeReleaseSpinlock(&listLock, prio);
    if(tcb == KeGetCurrentTask())
        KeTaskYield();
    return OK;
}

STATUS KeSleep(uint64_t time)
{
    return KePutTaskToSleep(KeGetCurrentTask(), time);
}

void KeDelay(uint64_t time)
{
    time += HalGetTimestamp();
    while(HalGetTimestamp() < time)
        ;
}

STATUS KeRefreshSleepingTasks(void)
{
    PRIO prio = KeAcquireSpinlock(&listLock);
    struct KeTaskControlBlock *s = NULL;
    uint64_t currentTimestamp = HalGetTimestamp();
    while(NULL != list)
    {
        s = list;
        if(currentTimestamp >= s->scheduling.block.timeout.until)
        {
            s->scheduling.block.timeout.until = 0;
            list = s->scheduling.next;
            s->scheduling.next = NULL;
            KeUnblockTask(s);
        }
        else
            break;
    }
    KeReleaseSpinlock(&listLock, prio);
    return OK;
}