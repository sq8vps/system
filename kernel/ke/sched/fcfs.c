#include "fcfs.h"
#include "ke/task/task.h"
#include "ke/core/mutex.h"

#define KE_FCFS_PRIORITIES 8

struct
{
    struct KeTaskControlBlock *head[KE_FCFS_PRIORITIES];
    struct KeTaskControlBlock *tail[KE_FCFS_PRIORITIES];
    KeSpinlock lock;
}
static KeFcfs = {.head = {nullptr}, .tail = {nullptr}, .lock = KeSpinlockInitializer};

void KeFcfsQueueTask(struct KeTaskControlBlock *tcb)
{
    PRIO queuePrio = KeAcquireDpcLevelSpinlock(&KeFcfs.lock);
    PRIO tcbPrio = KeAcquireDpcLevelSpinlock(&tcb->scheduling.lock);
    int32_t priority = tcb->scheduling.priority;
    if(priority < 0)
        priority = 0;
    else if(priority > (KE_FCFS_PRIORITIES - 1))
        priority = KE_FCFS_PRIORITIES - 1;

    if(nullptr != KeFcfs.head[priority])
    {
        KeFcfs.tail[priority]->scheduling.next = tcb;
        tcb->scheduling.previous = KeFcfs.tail[priority];
        tcb->scheduling.next = nullptr;
        KeFcfs.tail[priority] = tcb;
    }
    else
    {
        KeFcfs.head[priority] = tcb;
        KeFcfs.tail[priority] = tcb;
        tcb->scheduling.next = nullptr;
        tcb->scheduling.previous = nullptr;
    }
    
    tcb->scheduling.state = TASK_READY_TO_RUN;
    KeReleaseSpinlock(&tcb->scheduling.lock, tcbPrio);
    KeReleaseSpinlock(&KeFcfs.lock, queuePrio);
}

struct KeTaskControlBlock* KeFcfsGetNextTask(uint32_t cpu, uint64_t *slice)
{
    struct KeTaskControlBlock *tcb = nullptr;
    PRIO prio = KeAcquireDpcLevelSpinlock(&KeFcfs.lock);

    for(size_t i = KE_FCFS_PRIORITIES; i > 0; --i)
    {
        if(nullptr != KeFcfs.head[i - 1])
        {
            tcb = KeFcfs.head[i - 1];

            KeFcfs.head[i - 1] = tcb->scheduling.next;
            tcb->scheduling.previous = nullptr;
            tcb->scheduling.next = nullptr;
            *slice = 0;
            if(nullptr != KeFcfs.head[i - 1])
                KeFcfs.head[i - 1]->scheduling.previous = nullptr;
            break;
        }
    }

    KeReleaseSpinlock(&KeFcfs.lock, prio);

    return tcb;
}