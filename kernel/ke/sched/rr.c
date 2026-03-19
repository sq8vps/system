#include "rr.h"
#include "ke/task/task.h"
#include "ke/core/mutex.h"

#define KE_RR_PRIORITIES 8

#define KE_RR_DEFAULT_SLICE (10 * 1000 * 1000) //10 ms

struct
{
    struct
    {
        struct KeTaskControlBlock *head[KE_RR_PRIORITIES];
        struct KeTaskControlBlock *tail[KE_RR_PRIORITIES];
        KeSpinlock lock;
    } queue;

    uint64_t slice;
}
static KeRr = {.queue.head = {nullptr}, .queue.tail = {nullptr}, .queue.lock = KeSpinlockInitializer,
    .slice = KE_RR_DEFAULT_SLICE};


void KeRrQueueTask(struct KeTaskControlBlock *tcb)
{
    PRIO queuePrio = KeAcquireDpcLevelSpinlock(&KeRr.queue.lock);
    PRIO tcbPrio = KeAcquireDpcLevelSpinlock(&tcb->scheduling.lock);
    int32_t priority = tcb->scheduling.priority;
    if(priority < 0)
        priority = 0;
    else if(priority > (KE_RR_PRIORITIES - 1))
        priority = KE_RR_PRIORITIES - 1;

    if(nullptr != KeRr.queue.head[priority])
    {
        KeRr.queue.tail[priority]->scheduling.next = tcb;
        tcb->scheduling.previous = KeRr.queue.tail[priority];
        tcb->scheduling.next = nullptr;
        KeRr.queue.tail[priority] = tcb;
    }
    else
    {
        KeRr.queue.head[priority] = tcb;
        KeRr.queue.tail[priority] = tcb;
        tcb->scheduling.next = nullptr;
        tcb->scheduling.previous = nullptr;
    }
    
    tcb->scheduling.state = TASK_READY_TO_RUN;
    KeReleaseSpinlock(&tcb->scheduling.lock, tcbPrio);
    KeReleaseSpinlock(&KeRr.queue.lock, queuePrio);
}

struct KeTaskControlBlock* KeRrGetNextTask(uint32_t cpu, uint64_t *slice)
{
    struct KeTaskControlBlock *tcb = nullptr;
    PRIO prio = KeAcquireDpcLevelSpinlock(&KeRr.queue.lock);

    for(size_t i = KE_RR_PRIORITIES; i > 0; --i)
    {
        if(nullptr != KeRr.queue.head[i - 1])
        {
            tcb = KeRr.queue.head[i - 1];

            KeRr.queue.head[i - 1] = tcb->scheduling.next;
            tcb->scheduling.previous = nullptr;
            tcb->scheduling.next = nullptr;
            *slice = KeRr.slice;
            if(nullptr != KeRr.queue.head[i - 1])
                KeRr.queue.head[i - 1]->scheduling.previous = nullptr;
            break;
        }
    }

    KeReleaseSpinlock(&KeRr.queue.lock, prio);

    return tcb;
}