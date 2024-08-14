#include "sleep.h"
#include "sched.h"
#include "hal/time.h"
#include "ke/core/mutex.h"
#include "ke/task/task.h"

static struct KeTaskControlBlock *list = NULL;

static KeSpinlock listLock = KeSpinlockInitializer;

STATUS KePutTaskToSleep(struct KeTaskControlBlock *tcb, uint64_t time)
{
    KeBlockTask(tcb, TASK_BLOCK_SLEEP);
    tcb->waitUntil = HalGetTimestamp() + time;

    PRIO prio = KeAcquireSpinlock(&listLock);
    if(NULL == list)
        list = tcb;
    else
    {
        struct KeTaskControlBlock *s = list;
        //keep list sorted: the earliest deadline is first
        while(NULL != s->next)
        {
            if((s->waitUntil <= tcb->waitUntil) && (s->next->waitUntil > tcb->waitUntil))
            {
                tcb->next = s->next;
                break;
            }
            s = s->next;
        }
        s->next = tcb;
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
        if(currentTimestamp >= s->waitUntil)
        {
            s->waitUntil = 0;
            list = s->next;
            s->next = NULL;
            KeUnblockTask(s);
        }
        else
            break;
    }
    KeReleaseSpinlock(&listLock, prio);
    return OK;
}