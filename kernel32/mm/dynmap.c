#include "dynmap.h"
#include "avl.h"
#include "heap.h"
#include "ke/core/panic.h"
#include "ke/core/mutex.h"
#include "hal/arch.h"

#define MM_DYNAMIC_MEMORY_SPLIT_THRESHOLD (MM_PAGE_SIZE) //dynamic memory region split threshold when requested block is smaller

static struct MmAvlNode *dynamicMemoryTree[3] = {NULL, NULL, NULL};
#define MM_DYNAMIC_ADDRESS_FREE_TREE dynamicMemoryTree[0]
#define MM_DYNAMIC_SIZE_FREE_TREE dynamicMemoryTree[1]
#define MM_DYNAMIC_ADDRESS_USED_TREE dynamicMemoryTree[2]

static KeSpinlock dynamicAllocatorMutex = KeSpinlockInitializer;

void *MmReserveDynamicMemory(uintptr_t n)
{
    n = ALIGN_UP(n, MM_PAGE_SIZE);
    
    PRIO prio = KeAcquireSpinlock(&dynamicAllocatorMutex);

    struct MmAvlNode *region = MmAvlFindGreaterOrEqual(MM_DYNAMIC_SIZE_FREE_TREE, n);
    if(NULL == region) //no fitting region available
    {
        KeReleaseSpinlock(&dynamicAllocatorMutex, prio);
        return NULL;
    }
    //else region is available
    uintptr_t vAddress = region->buddy->key; //store its address
    uintptr_t remainingSize = region->key - n; //calculate remaining size

    //delete old nodes
    struct MmAvlNode *buddy = region->buddy;
    MmAvlDelete(&MM_DYNAMIC_SIZE_FREE_TREE, region);
    MmAvlDelete(&MM_DYNAMIC_ADDRESS_FREE_TREE, buddy);

    //check if it should be split

    if(remainingSize >= MM_DYNAMIC_MEMORY_SPLIT_THRESHOLD)
    {
        region = MmAvlInsertPair(&MM_DYNAMIC_SIZE_FREE_TREE, &MM_DYNAMIC_ADDRESS_FREE_TREE, remainingSize, vAddress + n);
        if(NULL == region)
            goto MmMapDynamicMemoryFail;
    }

    struct MmAvlNode *node = MmAvlInsert(&MM_DYNAMIC_ADDRESS_USED_TREE, vAddress);
    if(NULL == node)
    {
        MmAvlDelete(&MM_DYNAMIC_ADDRESS_FREE_TREE, region->buddy);
        MmAvlDelete(&MM_DYNAMIC_SIZE_FREE_TREE, region);
        goto MmMapDynamicMemoryFail;  
    }

    node->val = n; //store size

    KeReleaseSpinlock(&dynamicAllocatorMutex, prio);
    return (void*)vAddress;

MmMapDynamicMemoryFail:
    KeReleaseSpinlock(&dynamicAllocatorMutex, prio);
    return NULL;
}

static uintptr_t MmFreeDynamicMemoryReservationEx(void *ptr, bool unmap)
{
    ptr = (void*)ALIGN_DOWN((uintptr_t)ptr, MM_PAGE_SIZE);

    PRIO prio = KeAcquireSpinlock(&dynamicAllocatorMutex);
    struct MmAvlNode *this = MmAvlFindExactMatch(MM_DYNAMIC_ADDRESS_USED_TREE, (uintptr_t)ptr);
    if(NULL == this)
    {
        KeReleaseSpinlock(&dynamicAllocatorMutex, prio);
        return 0;
    }

    uintptr_t n = this->val; //get size
    uintptr_t originalSize = n;
    void *originalPtr = ptr;
    MmAvlDelete(&MM_DYNAMIC_ADDRESS_USED_TREE, this);

    //check if there is an adjacent free region with higher address
    struct MmAvlNode *node = MmAvlFindExactMatch(MM_DYNAMIC_ADDRESS_FREE_TREE, (uintptr_t)ptr + n);
    if(NULL != node)
    {
        n += node->buddy->key; //sum size
        MmAvlDelete(&MM_DYNAMIC_SIZE_FREE_TREE, node->buddy);
        MmAvlDelete(&MM_DYNAMIC_ADDRESS_FREE_TREE, node);
    }
    //check if there is an adjacent preceeding free region
    node = MmAvlFindLess(MM_DYNAMIC_ADDRESS_FREE_TREE, (uintptr_t)ptr);
    if(NULL != node)
    {
        n += node->buddy->key; //sum size
        ptr = (void*)(node->key); //store new address
        MmAvlDelete(&MM_DYNAMIC_SIZE_FREE_TREE, node->buddy);
        MmAvlDelete(&MM_DYNAMIC_ADDRESS_FREE_TREE, node);
    }

    //TODO: should be handled somehow if NULL returned
    MmAvlInsertPair(&MM_DYNAMIC_SIZE_FREE_TREE, &MM_DYNAMIC_ADDRESS_FREE_TREE, n, (uintptr_t)ptr);

    if(unmap && (0 != originalSize))
        HalUnmapMemoryEx((uintptr_t)originalPtr, originalSize);

    KeReleaseSpinlock(&dynamicAllocatorMutex, prio);
    return originalSize;
}

uintptr_t MmFreeDynamicMemoryReservation(void *ptr)
{
    return MmFreeDynamicMemoryReservationEx(ptr, false);
}

void *MmMapDynamicMemory(uintptr_t pAddress, uintptr_t n, MmMemoryFlags flags)
{
    n = ALIGN_UP(pAddress + n, MM_PAGE_SIZE) - ALIGN_DOWN(pAddress, MM_PAGE_SIZE);

    void *ptr = MmReserveDynamicMemory(n);
    if(NULL == ptr)
        return NULL;
    
    STATUS ret;
    if(OK != (ret = HalMapMemoryEx(ALIGN_DOWN((uintptr_t)ptr, MM_PAGE_SIZE), 
        ALIGN_DOWN(pAddress, MM_PAGE_SIZE), 
        n, 
        MM_FLAG_WRITABLE | flags)))
    {
        MmFreeDynamicMemoryReservation(ptr);
        return NULL;
    }
    return (void*)((uintptr_t)ptr + (pAddress % MM_PAGE_SIZE));
}

void MmUnmapDynamicMemory(void *ptr)
{
    if(NULL == ptr)
        return;
    MmFreeDynamicMemoryReservationEx(ptr, true);
}

void MmInitDynamicMemory(struct KernelEntryArgs *kernelArgs)
{
    if(0 != kernelArgs->initrdSize)
    {
        //align size to page size
        kernelArgs->initrdSize = ALIGN_UP(kernelArgs->initrdSize, MM_PAGE_SIZE);
        
        if(kernelArgs->initrdAddress & (MM_PAGE_SIZE - 1)) //address not aligned
            KePanicEx(BOOT_FAILURE, MM_DYNAMIC_MEMORY_INIT_FAILURE, MM_UNALIGNED_MEMORY, 0, 0);

        if(MM_DYNAMIC_START_ADDRESS <= kernelArgs->initrdAddress)
        {   
            uintptr_t dummy;
            for(uint32_t i = 0; i < (kernelArgs->initrdSize / MM_PAGE_SIZE + ((kernelArgs->initrdSize % MM_PAGE_SIZE) > 0)); i++)
            {
                //check if pages are actually mapped
                if(OK != HalGetPhysicalAddress(kernelArgs->initrdAddress + (i * MM_PAGE_SIZE), &dummy))
                    KePanicEx(BOOT_FAILURE, MM_DYNAMIC_MEMORY_INIT_FAILURE, 0, 0, 0);
            }

            struct MmAvlNode *node =  MmAvlInsert(&MM_DYNAMIC_ADDRESS_USED_TREE, kernelArgs->initrdAddress);
            if(NULL == node)
                KePanicEx(BOOT_FAILURE, MM_DYNAMIC_MEMORY_INIT_FAILURE, OUT_OF_RESOURCES, 0, 0);
            node->val = kernelArgs->initrdSize;

            if(kernelArgs->initrdAddress - MM_DYNAMIC_START_ADDRESS)
            {
                if(NULL == MmAvlInsertPair(&MM_DYNAMIC_ADDRESS_FREE_TREE, &MM_DYNAMIC_SIZE_FREE_TREE, 
                MM_DYNAMIC_START_ADDRESS, kernelArgs->initrdAddress - MM_DYNAMIC_START_ADDRESS))
                    KePanicEx(BOOT_FAILURE, MM_DYNAMIC_MEMORY_INIT_FAILURE, OUT_OF_RESOURCES, 0, 0);
            }
        }
        else if(MM_DYNAMIC_START_ADDRESS > kernelArgs->initrdAddress)
            KePanicEx(BOOT_FAILURE, MM_DYNAMIC_MEMORY_INIT_FAILURE, 0, 0, 0);
    }
    else
        kernelArgs->initrdAddress = MM_DYNAMIC_START_ADDRESS;
    
    uintptr_t remaining = MM_DYNAMIC_MAX_SIZE - (kernelArgs->initrdAddress + kernelArgs->initrdSize - MM_DYNAMIC_START_ADDRESS);
    if(remaining)
    {
        if(NULL == MmAvlInsertPair(&MM_DYNAMIC_ADDRESS_FREE_TREE, &MM_DYNAMIC_SIZE_FREE_TREE, 
            kernelArgs->initrdAddress + kernelArgs->initrdSize, remaining))
                KePanicEx(BOOT_FAILURE, MM_DYNAMIC_MEMORY_INIT_FAILURE, OUT_OF_RESOURCES, 0, 0);
    }
}


