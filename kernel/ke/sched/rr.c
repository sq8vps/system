#include "rr.h"
#include "ke/task/task.h"
#include "ke/core/mutex.h"

#define KE_RR_PRIORITIES 8

#define KE_RR_DEFAULT_SLICE (10 * 1000 * 1000) //10 ms

static struct
{
    struct
    {
        struct KeTaskControlBlock *head[KE_RR_PRIORITIES];
        struct KeTaskControlBlock *tail[KE_RR_PRIORITIES];
        KeSpinlock lock;
    } queue;

    uint64_t slice;
}
KeRr = {.queue.head = {nullptr}, .queue.tail = {nullptr}, .queue.lock = KeSpinlockInitializer,
    .slice = KE_RR_DEFAULT_SLICE};


void KeRrQueueTask(struct KeTaskControlBlock *tcb)
{
    PRIO prio = KeAcquireSpinlock(&KeRr.queue.lock);
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
    KeReleaseSpinlock(&KeRr.queue.lock, prio);
}

struct KeTaskControlBlock* KeRrGetNextTask(uint32_t cpu, struct KeTaskControlBlock *current, uint64_t *slice)
{
    if(likely(nullptr != current) && (KE_SCHED_RR != current->scheduling.policy))
        current = nullptr;

    struct KeTaskControlBlock *next = nullptr;
    PRIO prio = KeAcquireSpinlock(&KeRr.queue.lock);

    for(size_t i = KE_RR_PRIORITIES; i > 0; --i)
    {
        if((nullptr != current) && (TASK_RUNNING == current->scheduling.state) && (current->scheduling.priority > (int32_t)i))
        {
            next = current;
            *slice = KeRr.slice;
            break;
        }

        if(nullptr != KeRr.queue.head[i - 1])
        {
            next = KeRr.queue.head[i - 1];

            KeRr.queue.head[i - 1] = next->scheduling.next;
            next->scheduling.previous = nullptr;
            next->scheduling.next = nullptr;
            *slice = KeRr.slice;
            if(nullptr != KeRr.queue.head[i - 1])
                KeRr.queue.head[i - 1]->scheduling.previous = nullptr;
            break;
        }
    }

    if((nullptr == next) && (nullptr != current) && (TASK_RUNNING == current->scheduling.state))
    {
        //last resort, absolutely nothing else, but the currently running task can run further
        next = current;
        *slice = KeRr.slice;
    }

    KeReleaseSpinlock(&KeRr.queue.lock, prio);

    return next;
}