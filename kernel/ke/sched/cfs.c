#include "cfs.h"
#include "ke/task/task.h"
#include "ke/core/mutex.h"
#include <stdckdint.h>

#define BST_PROVIDE_ABSTRACTION 1
#include "rtl/bst.h"

#define KE_CFS_BASE_TIMESLICE MS_TO_NS(20) /**< Base timeslice */
#define KE_CFS_MINIMUM_TIMESLICE MS_TO_NS(1) /**< Minimum timeslice that can be assigned to a task */

#define KE_CFS_PRIORITY_MIN -19
#define KE_CFS_PRIORITY_MAX 20

static const uint64_t KeCfsPrioToWeight[40] = {
    15, 18, 23, 29, 36, 
    45, 56, 70, 87, 110, 
    137, 172, 215, 272, 335, 
    423, 526, 655, 820, 1024, 
    1277, 1586, 1991, 2501, 3121, 
    3906, 4904, 6100, 7620, 9548, 
    11916, 14949, 18705, 23254, 29154, 
    36291, 46273, 56483, 71755, 88761, 
};

static const uint64_t KeCfsPrioToInvWeight[40] = {
    286331153, 238609294, 186737708, 148102320, 119304647, 
    95443717, 76695844, 61356676, 49367440, 39045157, 
    31350126, 24970740, 19976592, 15790321, 12820798, 
    10153587, 8165337, 6557202, 5237765, 4194304, 
    3363326, 2708050, 2157191, 1717300, 1376151, 
    1099582, 875809, 704093, 563644, 449829, 
    360437, 287308, 229616, 184698, 147320, 
    118348, 92818, 76040, 59856, 48388,
};

struct KeCfsData
{
    struct TreeNode tree;
    struct KeTaskControlBlock *tcb;
    uint64_t vruntime;
    uint64_t weight;
    bool leftmost;
};

struct
{
    struct TreeNode *tree;
    struct KeTaskControlBlock *leftmost;
    uint64_t totalWeight;
    KeSpinlock lock;

    uint64_t baseSlice;
}
static KeCfs = {.tree = {nullptr}, .leftmost = nullptr, .totalWeight = 1, .lock = KeSpinlockInitializer,
    .baseSlice = KE_CFS_BASE_TIMESLICE};

static int KeCfsCompareNodes(struct TreeNode *A, struct TreeNode *B)
{
    const struct KeCfsData *dA = (const struct KeCfsData*)A->aux.v;
    const struct KeCfsData *dB = (const struct KeCfsData*)B->aux.v;

    if(dA->vruntime < dB->vruntime)
        return -1;
    else if(dA->vruntime > dB->vruntime)
        return 1;
    else
        return 0;
}

static void KeCfsSetNewVruntime(struct KeTaskControlBlock *tcb)
{
    struct KeCfsData *d = (struct KeCfsData*)tcb->scheduling.shadow;
    uint64_t result = 0;
    if(!ckd_mul(&result, KeCfsPrioToInvWeight[tcb->scheduling.priority - KE_CFS_PRIORITY_MIN], tcb->scheduling.lastRuntime))
    {
        result >>= 22;
        if(!ckd_add(&result, result, d->vruntime))
            d->vruntime = result;
        else
            d->vruntime = UINT64_MAX;
    }
    else
        d->vruntime = UINT64_MAX;
}

void KeCfsQueueTask(struct KeTaskControlBlock *tcb)
{
    PRIO treePrio = KeAcquireDpcLevelSpinlock(&KeCfs.lock);
    PRIO tcbPrio = KeAcquireDpcLevelSpinlock(&tcb->scheduling.lock);

    struct KeCfsData *d = (struct KeCfsData*)tcb->scheduling.shadow;
    int32_t priority = tcb->scheduling.priority;
    if(unlikely(priority < KE_CFS_PRIORITY_MIN))
        tcb->scheduling.priority = KE_CFS_PRIORITY_MIN;
    else if(unlikely(priority > KE_CFS_PRIORITY_MAX))
        tcb->scheduling.priority = KE_CFS_PRIORITY_MAX;

    d->weight = KeCfsPrioToWeight[priority];
    KeCfsSetNewVruntime(tcb);

    d->tree.aux.v = d;
    d->tcb = tcb;
    if(-1 == TreeInsertEx(&KeCfs.tree, &d->tree, KeCfsCompareNodes))
        KeCfs.leftmost = tcb;

    KeCfs.totalWeight += d->weight;
    
    tcb->scheduling.state = TASK_READY_TO_RUN;
    KeReleaseSpinlock(&tcb->scheduling.lock, tcbPrio);
    KeReleaseSpinlock(&KeCfs.lock, treePrio);
}

static void KeCfsSetNewLeftmost(void)
{
    struct TreeNode *t = KeCfs.tree;
    if(nullptr != t)
    {
        while(nullptr != t->left)
            t = t->left;

        KeCfs.leftmost = ((struct KeCfsData*)t->left->aux.v)->tcb;
    }
    else
       KeCfs.leftmost = nullptr; 
}

struct KeTaskControlBlock* KeCfsGetNextTask(uint32_t cpu, uint64_t *slice)
{
    struct KeTaskControlBlock *tcb = nullptr;

    PRIO prio = KeAcquireDpcLevelSpinlock(&KeCfs.lock);

    tcb = KeCfs.leftmost;
    struct KeCfsData *d = (struct KeCfsData*)tcb->scheduling.shadow;
    KeCfs.tree = TreeRemoveEx(KeCfs.tree, tcb, KeCfsCompareNodes);
    KeCfsSetNewLeftmost();

    *slice = KeCfs.baseSlice * d->weight / KeCfs.totalWeight;
    if(*slice < KE_CFS_MINIMUM_TIMESLICE)
        *slice = KE_CFS_MINIMUM_TIMESLICE;

    KeCfs.totalWeight -= d->weight;
    
    KeReleaseSpinlock(&KeCfs.lock, prio);

    return tcb;
}