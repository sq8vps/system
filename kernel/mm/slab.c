#include "slab.h"
#include "mm/heap.h"
#include "ke/core/mutex.h"
#include "assert.h"
#include "hal/arch.h"
#include "mm/palloc.h"
#include "mm/dynmap.h"

struct MmSlabEntry
{
    struct MmSlabEntry *next;
};

struct MmSlabEntryEx
{
    struct MmSlabEntryEx *next;
    PADDRESS physical;
};

struct MmSlab
{
    size_t chunkSize;
    size_t chunkCount;
    uint32_t pool;
    size_t boundary;
    uint32_t fromPool : 1;
    union
    {
        struct MmSlabEntry *freeStack;
        struct MmSlabEntryEx *freeStackEx;
    };
    KeSpinlock lock;
};

static STATUS MmSlabAllocateBlock(struct MmSlab *slab)
{
    if(slab->fromPool)
    {
        PADDRESS physical = 0;
        struct MmSlabEntryEx *s = nullptr;
        if(MmAllocatePhysicalMemoryFromPool(PAGE_SIZE, &physical, slab->pool) < PAGE_SIZE)
        {
            return OUT_OF_RESOURCES;
        }

        s = MmMapDynamicMemory(physical, PAGE_SIZE, MM_FLAG_WRITE_THROUGH | MM_FLAG_CACHE_DISABLE);
        if(nullptr == s)
        {
            MmFreePhysicalMemory(physical, PAGE_SIZE);
            return OUT_OF_RESOURCES;
        }

        struct MmSlabEntryEx *t = s;
        for(size_t i = 0; i < (slab->chunkCount - 1); i++)
        {
            t->physical = physical;
            t->next = (struct MmSlabEntryEx*)((uintptr_t)t + slab->chunkSize);
            t = t->next;
            physical += slab->chunkSize;
        }
        t->physical = physical;
        t->next = NULL;

        if(NULL == slab->freeStackEx)
        {
            slab->freeStackEx = s;
        }
        else
        {
            t->next = slab->freeStackEx;
            slab->freeStackEx = t;
        }
    }
    else
    {
        struct MmSlabEntry *s = MmAllocateKernelHeap(slab->chunkCount * slab->chunkSize);
        if(NULL == s)
            return OUT_OF_RESOURCES;

        struct MmSlabEntry *t = s;
        for(size_t i = 0; i < (slab->chunkCount - 1); i++)
        {
            t->next = (struct MmSlabEntry*)((uintptr_t)t + slab->chunkSize);
            t = t->next;
        }
        t->next = NULL;

        if(NULL == slab->freeStack)
        {
            slab->freeStack = s;
        }
        else
        {
            t->next = slab->freeStack;
            slab->freeStack = t;
        }
    }
    return OK;

}

void *MmSlabCreate(size_t chunkSize, size_t chunkCount)
{
    if((0 == chunkCount) || (0 == chunkSize))
        return NULL;

    if(chunkSize < sizeof(struct MmSlabEntry))
        chunkSize = sizeof(struct MmSlabEntry);

    struct MmSlab *slab = MmAllocateKernelHeapZeroed(sizeof(*slab));
    if(NULL == slab)
        return NULL;
    
    slab->chunkCount = chunkCount;
    slab->chunkSize = chunkSize;
    slab->fromPool = 0;

    return slab;
}

void *MmSlabAllocate(void *slabHandle)
{
    struct MmSlab *slab = slabHandle;
    struct MmSlabEntry *e = NULL;

    if(slab->fromPool)
        return nullptr;

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
    slab->freeStack = e->next;
    barrier();
    KeReleaseSpinlock(&(slab->lock), prio);
    return e;
}

void MmSlabFree(void *slabHandle, void *memory)
{
    if(NULL == memory)
        return;
    
    struct MmSlab *slab = slabHandle;
    struct MmSlabEntry *entry = memory;

    if(slab->fromPool)
        return;

    PRIO prio = KeAcquireSpinlock(&(slab->lock));
    barrier();
    entry->next = slab->freeStack;
    slab->freeStack = entry;
    barrier();
    KeReleaseSpinlock(&(slab->lock), prio);
}

void MmSlabDestroy(void *slabHandle)
{
    UNUSED(slabHandle);
    //TODO: implement!!!!
}

void *MmSlabAllocateP(void *slabHandle, PADDRESS *physical)
{
    struct MmSlab *slab = slabHandle;
    struct MmSlabEntryEx *e = NULL;

    if(!slab->fromPool)
        return nullptr;

    PRIO prio = KeAcquireSpinlock(&(slab->lock));
    barrier();
    if(nullptr == slab->freeStackEx)
    {
        if(OK != MmSlabAllocateBlock(slab))
        {
            barrier();
            KeReleaseSpinlock(&(slab->lock), prio);
            return NULL;
        }
    }

    e = slab->freeStackEx;
    *physical = e->physical;
    slab->freeStackEx = e->next;
    barrier();
    KeReleaseSpinlock(&(slab->lock), prio);
    return e;
}

void MmSlabFreeP(void *slabHandle, void *memory, PADDRESS physical)
{
    struct MmSlab *slab = slabHandle;
    struct MmSlabEntryEx *entry = memory;

    if(nullptr == memory)
        return;

    if(!slab->fromPool)
        return;
    
    PRIO prio = KeAcquireSpinlock(&(slab->lock));
    barrier();
    entry->next = slab->freeStackEx;
    slab->freeStackEx = entry;
    entry->physical = physical;
    barrier();
    KeReleaseSpinlock(&(slab->lock), prio);
}

void *MmSlabCreateP(size_t chunkSize, uint32_t pool, size_t boundary)
{
    if((pool > HAL_PHYSICAL_MEMORY_POOLS) || (0 == chunkSize) || (chunkSize > PAGE_SIZE))
        return nullptr;

    if(chunkSize < sizeof(struct MmSlabEntry))
        chunkSize = sizeof(struct MmSlabEntry);

    if((0 != boundary) && (boundary < chunkSize))
        return nullptr;

    struct MmSlab *slab = MmAllocateKernelHeapZeroed(sizeof(*slab));
    if(NULL == slab)
        return NULL;
    
    slab->chunkCount = PAGE_SIZE / chunkSize;
    slab->chunkSize = chunkSize;
    slab->fromPool = 1;
    slab->pool = pool;
    slab->boundary = boundary;

    return slab;    
}