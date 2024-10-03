#include "slab.h"
#include "mm/heap.h"
#include "ke/core/mutex.h"
#include "assert.h"

struct MmSlabEntry
{
    struct MmSlabEntry *next;
    uint32_t free;
};

struct MmSlab
{
    uintptr_t chunkSize;
    uintptr_t chunkCount;
    struct MmSlab *nextSlab;
    struct MmSlabEntry *freeStack;
    KeSpinlock lock;
};

static STATUS MmSlabAllocateBlock(struct MmSlab *slab)
{
    struct MmSlabEntry *s = MmAllocateKernelHeap(slab->chunkCount * (slab->chunkSize + sizeof(struct MmSlabEntry)));
    if(NULL == s)
        return OUT_OF_RESOURCES;

    struct MmSlabEntry *t = s;
    for(uintptr_t i = 0; i < (slab->chunkCount - 1); i++)
    {
        t->next = (struct MmSlabEntry*)((uintptr_t)t + slab->chunkSize + sizeof(*t));
        t->free = 1;
        t = t->next;
    }
    t->next = NULL;
    t->free = 1;

    if(NULL == slab->freeStack)
    {
        slab->freeStack = s;
    }
    else
    {
        t->next = slab->freeStack;
        slab->freeStack = t;
    }
    return OK;
}

void *MmSlabCreate(uintptr_t chunkSize, uintptr_t chunkCount)
{
    if((0 == chunkCount) || (0 == chunkSize))
        return NULL;

    struct MmSlab *slab = MmAllocateKernelHeapZeroed(sizeof(struct MmSlab));
    if(NULL == slab)
        return NULL;
    
    slab->chunkCount = chunkCount;
    slab->chunkSize = chunkSize;

    return slab;
}

void *MmSlabAllocate(void *slabHandle)
{
    struct MmSlab *slab = slabHandle;
    struct MmSlabEntry *e = NULL;

    PRIO prio = KeAcquireSpinlock(&(slab->lock));
    barrier();
    if(NULL == slab->freeStack)
    {
        if(OK != MmSlabAllocateBlock(slab))
        {
            barrier();
            KeReleaseSpinlock(&(slab->lock), prio);
            return NULL;
        }
    }

    e = slab->freeStack;
    ASSERT(e->free);
    slab->freeStack = e->next;
    e->free = 0;
    barrier();
    KeReleaseSpinlock(&(slab->lock), prio);
    return e + 1;
}

void MmSlabFree(void *slabHandle, void *memory)
{
    if(NULL == memory)
        return;
    
    struct MmSlab *slab = slabHandle;
    struct MmSlabEntry *entry = memory;
    entry--; //jump to header start

    ASSERT(!entry->free);
    PRIO prio = KeAcquireSpinlock(&(slab->lock));
    barrier();
    entry->next = slab->freeStack;
    slab->freeStack = entry;
    entry->free = 1;
    barrier();
    KeReleaseSpinlock(&(slab->lock), prio);
}