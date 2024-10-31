#include "dynmap.h"
#include "bst.h"
#include "heap.h"
#include "ke/core/panic.h"
#include "ke/core/mutex.h"
#include "hal/arch.h"
#include "hal/mm.h"

#define MM_DYNAMIC_MEMORY_SPLIT_THRESHOLD (PAGE_SIZE) //dynamic memory region split threshold when requested block is smaller

static struct BstNode *MmDynamicMemoryTree[3] = {NULL, NULL, NULL};
#define MM_DYNAMIC_ADDRESS_FREE_TREE MmDynamicMemoryTree[0]
#define MM_DYNAMIC_SIZE_FREE_TREE MmDynamicMemoryTree[1]
#define MM_DYNAMIC_ADDRESS_USED_TREE MmDynamicMemoryTree[2]

static KeSpinlock MmDynamicAllocatorLock = KeSpinlockInitializer;

void *MmReserveDynamicMemory(uintptr_t n)
{
    n = ALIGN_UP(n, PAGE_SIZE);
    
    PRIO prio = KeAcquireSpinlock(&MmDynamicAllocatorLock);
    barrier();

    struct BstNode *region = BstFindGreaterOrEqual(MM_DYNAMIC_SIZE_FREE_TREE, n);
    if(NULL == region) //no fitting region available
    {
        KeReleaseSpinlock(&MmDynamicAllocatorLock, prio);
        return NULL;
    }
    //else region is available
    uintptr_t vAddress = region->buddy->key; //store its address
    uintptr_t remainingSize = region->key - n; //calculate remaining size

    struct BstNode *node = BstInsert(&MM_DYNAMIC_ADDRESS_USED_TREE, vAddress);
    if(NULL == node)
    {
        goto MmMapDynamicMemoryFail;  
    }

    node->val = region->key; //store size

    //delete old nodes
    BstDelete(&MM_DYNAMIC_ADDRESS_FREE_TREE, region->buddy);
    BstDelete(&MM_DYNAMIC_SIZE_FREE_TREE, region);

    //check if it should be split
    if(remainingSize >= MM_DYNAMIC_MEMORY_SPLIT_THRESHOLD)
    {
        region = BstInsertPair(&MM_DYNAMIC_SIZE_FREE_TREE, &MM_DYNAMIC_ADDRESS_FREE_TREE, remainingSize, vAddress + n);
        if(NULL != region)
            node->val = n;
    }

    barrier();
    KeReleaseSpinlock(&MmDynamicAllocatorLock, prio);
    return (void*)vAddress;

MmMapDynamicMemoryFail:
    barrier();
    KeReleaseSpinlock(&MmDynamicAllocatorLock, prio);
    return NULL;
}

static uintptr_t MmFreeDynamicMemoryReservationEx(const void *ptr, bool unmap)
{
    ptr = (void*)ALIGN_DOWN((uintptr_t)ptr, PAGE_SIZE);

    PRIO prio = KeAcquireSpinlock(&MmDynamicAllocatorLock);
    barrier();
    struct BstNode *this = BstFindExactMatch(MM_DYNAMIC_ADDRESS_USED_TREE, (uintptr_t)ptr);
    if(NULL == this)
    {
        KeReleaseSpinlock(&MmDynamicAllocatorLock, prio);
        return 0;
    }

    uintptr_t n = this->val; //get size
    uintptr_t originalSize = n;
    const void *originalPtr = ptr;
    BstDelete(&MM_DYNAMIC_ADDRESS_USED_TREE, this);

    //check if there is an adjacent free region with higher address
    struct BstNode *node = BstFindExactMatch(MM_DYNAMIC_ADDRESS_FREE_TREE, (uintptr_t)ptr + n);
    if(NULL != node)
    {
        n += node->buddy->key; //sum size
        BstDelete(&MM_DYNAMIC_SIZE_FREE_TREE, node->buddy);
        BstDelete(&MM_DYNAMIC_ADDRESS_FREE_TREE, node);
    }
    //check if there is an adjacent preceeding free region
    node = BstFindLess(MM_DYNAMIC_ADDRESS_FREE_TREE, (uintptr_t)ptr);
    if((NULL != node)
        && ((node->key + node->buddy->key) == (uintptr_t)ptr))
    {
        n += node->buddy->key; //sum size
        ptr = (void*)(node->key); //store new address
        BstDelete(&MM_DYNAMIC_SIZE_FREE_TREE, node->buddy);
        BstDelete(&MM_DYNAMIC_ADDRESS_FREE_TREE, node);
    }

    //TODO: should be handled somehow if NULL returned
    if(NULL == BstInsertPair(&MM_DYNAMIC_SIZE_FREE_TREE, &MM_DYNAMIC_ADDRESS_FREE_TREE, n, (uintptr_t)ptr))
        KePanic(UNEXPECTED_FAULT);

    if(unmap && (0 != originalSize))
        if(OK != HalUnmapMemoryEx((uintptr_t)originalPtr, originalSize))
            originalSize = 0;

    barrier();
    KeReleaseSpinlock(&MmDynamicAllocatorLock, prio);
    return originalSize;
}

uintptr_t MmFreeDynamicMemoryReservation(const void *ptr)
{
    return MmFreeDynamicMemoryReservationEx(ptr, false);
}

void *MmMapDynamicMemory(uintptr_t pAddress, uintptr_t n, MmMemoryFlags flags)
{
    n = ALIGN_UP(pAddress + n, PAGE_SIZE) - ALIGN_DOWN(pAddress, PAGE_SIZE);

    void *ptr = MmReserveDynamicMemory(n);
    if(NULL == ptr)
        return NULL;
    
    STATUS ret;
    if(OK != (ret = HalMapMemoryEx(ALIGN_DOWN((uintptr_t)ptr, PAGE_SIZE), 
        ALIGN_DOWN(pAddress, PAGE_SIZE), 
        n, 
        MM_FLAG_WRITABLE | flags)))
    {
        MmFreeDynamicMemoryReservation(ptr);
        return NULL;
    }
    return (void*)((uintptr_t)ptr + (pAddress % PAGE_SIZE));
}

void MmUnmapDynamicMemory(const void *ptr)
{
    if(NULL == ptr)
        return;
    MmFreeDynamicMemoryReservationEx(ptr, true);
}

void MmInitDynamicMemory(void)
{
    if(NULL == BstInsertPair(&MM_DYNAMIC_ADDRESS_FREE_TREE, &MM_DYNAMIC_SIZE_FREE_TREE, 
        HalGetDynamicSpaceBase(), HalGetDynamicSpaceSize()))
        KePanicEx(BOOT_FAILURE, 0, OUT_OF_RESOURCES, 0, 0);
}


