#include "dynmap.h"
#include "avl.h"
#include "valloc.h"
#include "heap.h"
#include "../ke/panic.h"
#include "../ke/mutex.h"

#define MM_DYNAMIC_MEMORY_SPLIT_THRESHOLD (MM_PAGE_SIZE) //dynamic memory region split threshold when requested block is smaller

struct MmAvlNode *dynamicMemoryTree[2] = {NULL, NULL};
#define MM_DYNAMIC_ADDRESS_TREE dynamicMemoryTree[0]
#define MM_DYNAMIC_SIZE_TREE dynamicMemoryTree[1]

static KeSpinLock_t dynamicAllocatorMutex;

void *MmMapDynamicMemory(uintptr_t pAddress, uintptr_t n)
{
    if(pAddress & (MM_PAGE_SIZE - 1)) //address not page-aligned?
    {
        ERROR("physical address not page aligned\n");
        return NULL;
    }

    //align size to page size
    if(n % MM_PAGE_SIZE)
        n += (MM_PAGE_SIZE - (n % MM_PAGE_SIZE));

    KeAcquireSpinlockDisableIRQ(&dynamicAllocatorMutex);

    struct MmAvlNode *region = MmAvlFindFreeMemory(MM_DYNAMIC_SIZE_TREE, n);
    if(NULL == region) //no fitting region available
    {
        KeReleaseSpinlockEnableIRQ(&dynamicAllocatorMutex);
        ERROR("no dynamic memory region found\n");
        return NULL;
    }
    //else region is available
    uintptr_t vAddress = region->buddy->key; //store its address
    uintptr_t remainingSize = region->key - n; //calculate remaining size
    //perform mapping first
    STATUS ret = OK;
    if(OK != (ret = MmMapMemoryEx(vAddress, pAddress, n, MM_PAGE_FLAG_WRITABLE)))
    {
        KeReleaseSpinlockEnableIRQ(&dynamicAllocatorMutex);
        ERROR("memory mapping failed, error 0x%X\n", (uintptr_t)ret);
        return NULL;
    }
    //delete old nodes
    MmAvlDelete(&MM_DYNAMIC_SIZE_TREE, &MM_DYNAMIC_ADDRESS_TREE, region);

    //check if it should be split

    if(remainingSize >= MM_DYNAMIC_MEMORY_SPLIT_THRESHOLD)
    {
        if(NULL == MmAvlInsert(&MM_DYNAMIC_ADDRESS_TREE, &MM_DYNAMIC_SIZE_TREE, vAddress + n, remainingSize))
        {
            KeReleaseSpinlockEnableIRQ(&dynamicAllocatorMutex);
            ERROR("AVL tree insertion failed\n");
            return NULL;
        }
            //TODO: this should be handled somehow
    }
    KeReleaseSpinlockEnableIRQ(&dynamicAllocatorMutex);
    return (void*)(vAddress);
}

void MmUnmapDynamicMemory(void *ptr, uintptr_t n)
{
    //align size to page size
    if(n % MM_PAGE_SIZE)
        n += (MM_PAGE_SIZE - (n % MM_PAGE_SIZE));

    MmUnmapMemoryEx((uintptr_t)ptr, n);

    KeAcquireSpinlockDisableIRQ(&dynamicAllocatorMutex);

    //check if there is an adjacent free region with higher address
    struct MmAvlNode *node = MmAvlFindMemoryByAddress(MM_DYNAMIC_ADDRESS_TREE, (uintptr_t)ptr + n);
    if(NULL != node)
    {
        n += node->buddy->key; //sum size
        MmAvlDelete(&MM_DYNAMIC_ADDRESS_TREE, &MM_DYNAMIC_SIZE_TREE, node);
    }
    //check if there is an adjacent preceeding free region
    node = MmAvlFindHighestMemoryByAddressLimit(MM_DYNAMIC_ADDRESS_TREE, (uintptr_t)ptr);
    if(NULL != node)
    {
        n += node->buddy->key; //sum size
        ptr = (void*)(node->key); //store new address
        MmAvlDelete(&MM_DYNAMIC_ADDRESS_TREE, &MM_DYNAMIC_SIZE_TREE, node);
    }

    //TODO: should be handled somehow if NULL returned
    MmAvlInsert(&MM_DYNAMIC_ADDRESS_TREE, &MM_DYNAMIC_SIZE_TREE, (uintptr_t)ptr, n);
    KeReleaseSpinlockEnableIRQ(&dynamicAllocatorMutex);
}

void MmInitDynamicMemory(struct KernelEntryArgs *kernelArgs)
{
    if(0 != kernelArgs->initrdSize)
    {
        //align size to page size
        if(kernelArgs->initrdSize % MM_PAGE_SIZE)
            kernelArgs->initrdSize += (MM_PAGE_SIZE - (kernelArgs->initrdSize % MM_PAGE_SIZE));
        
        if(kernelArgs->initrdAddress & (MM_PAGE_SIZE - 1)) //address not aligned
            KePanicEx(MM_DYNAMIC_MEMORY_INIT_FAILURE, MM_UNALIGNED_MEMORY, 0, 0, 0);
        
        if(MM_DYNAMIC_START_ADDRESS < kernelArgs->initrdAddress)
        {
            if(NULL == MmAvlInsert(&MM_DYNAMIC_ADDRESS_TREE, &MM_DYNAMIC_SIZE_TREE, 
            MM_DYNAMIC_START_ADDRESS, kernelArgs->initrdAddress - MM_DYNAMIC_START_ADDRESS))
                KePanicEx(MM_DYNAMIC_MEMORY_INIT_FAILURE, MM_HEAP_ALLOCATION_FAILURE, 0, 0, 0);
        }
        else if(MM_DYNAMIC_START_ADDRESS > kernelArgs->initrdAddress)
            KePanic(MM_DYNAMIC_MEMORY_INIT_FAILURE);
    }
    else
        kernelArgs->initrdAddress = MM_DYNAMIC_START_ADDRESS;
    
    if(NULL == MmAvlInsert(&MM_DYNAMIC_ADDRESS_TREE, &MM_DYNAMIC_SIZE_TREE, 
    kernelArgs->initrdAddress + kernelArgs->initrdSize, 
    MM_DYNAMIC_MAX_SIZE - (kernelArgs->initrdAddress + kernelArgs->initrdSize - MM_DYNAMIC_START_ADDRESS)))
        KePanicEx(MM_DYNAMIC_MEMORY_INIT_FAILURE, MM_HEAP_ALLOCATION_FAILURE, 0, 0, 0);
}


