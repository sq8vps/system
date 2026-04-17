#include "fcfs.h"
#include "ke/task/task.h"
#include "ke/core/mutex.h"

#define KE_FCFS_PRIORITIES 8

static struct
{
    struct KeTaskControlBlock *head[KE_FCFS_PRIORITIES];
    struct KeTaskControlBlock *tail[KE_FCFS_PRIORITIES];
    KeSpinlock lock;
}
KeFcfs = {.head = {nullptr}, .tail = {nullptr}, .lock = KeSpinlockInitializer};

void KeFcfsQueueTask(struct KeTaskControlBlock *tcb)
{
    PRIO prio = KeAcquireSpinlock(&KeFcfs.lock);
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
    KeReleaseSpinlock(&KeFcfs.lock, prio);
}

struct KeTaskControlBlock* KeFcfsGetNextTask(uint32_t cpu, struct KeTaskControlBlock *current, uint64_t *slice)
{
    struct KeTaskControlBlock *next = nullptr;
    PRIO prio = KeAcquireSpinlock(&KeFcfs.lock);

    for(size_t i = KE_FCFS_PRIORITIES; i > 0; --i)
    {
        if(likely(nullptr != current) && (TASK_RUNNING == current->scheduling.state) && (current->scheduling.priority > (int32_t)i))
        {
            next = current;
            break;
        }

        if(nullptr != KeFcfs.head[i - 1])
        {
            next = KeFcfs.head[i - 1];

            KeFcfs.head[i - 1] = next->scheduling.next;
            next->scheduling.previous = nullptr;
            next->scheduling.next = nullptr;
            *slice = 0;
            if(nullptr != KeFcfs.head[i - 1])
                KeFcfs.head[i - 1]->scheduling.previous = nullptr;
            break;
        }
    }

    KeReleaseSpinlock(&KeFcfs.lock, prio);

    return next;
}