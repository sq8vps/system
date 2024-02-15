#include "dynmap.h"
#include "avl.h"
#include "heap.h"
#include "ke/core/panic.h"
#include "ke/core/mutex.h"

#define MM_DYNAMIC_MEMORY_SPLIT_THRESHOLD (MM_PAGE_SIZE) //dynamic memory region split threshold when requested block is smaller

static struct MmAvlNode *dynamicMemoryTree[2] = {NULL, NULL};
#define MM_DYNAMIC_ADDRESS_TREE dynamicMemoryTree[0]
#define MM_DYNAMIC_SIZE_TREE dynamicMemoryTree[1]

static KeSpinlock dynamicAllocatorMutex = KeSpinlockInitializer;

void *MmMapDynamicMemory(uintptr_t pAddress, uintptr_t n, MmPagingFlags_t flags)
{
    n = ALIGN_UP(n, MM_PAGE_SIZE);
    
    KeAcquireSpinlock(&dynamicAllocatorMutex);

    struct MmAvlNode *region = MmAvlFindFreeMemory(MM_DYNAMIC_SIZE_TREE, n);
    if(NULL == region) //no fitting region available
    {
        KeReleaseSpinlock(&dynamicAllocatorMutex);
        return NULL;
    }
    //else region is available
    uintptr_t vAddress = region->buddy->key; //store its address
    uintptr_t remainingSize = region->key - n; //calculate remaining size
    //perform mapping first
    STATUS ret = OK;
    if(OK != (ret = MmMapMemoryEx(vAddress, ALIGN_DOWN(pAddress, MM_PAGE_SIZE), n, MM_PAGE_FLAG_WRITABLE | flags)))
    {
        KeReleaseSpinlock(&dynamicAllocatorMutex);
        return NULL;
    }
    //delete old nodes
    MmAvlDelete(&MM_DYNAMIC_SIZE_TREE, &MM_DYNAMIC_ADDRESS_TREE, region);

    //check if it should be split

    if(remainingSize >= MM_DYNAMIC_MEMORY_SPLIT_THRESHOLD)
    {
        if(NULL == MmAvlInsert(&MM_DYNAMIC_ADDRESS_TREE, &MM_DYNAMIC_SIZE_TREE, vAddress + n, remainingSize))
        {
            KeReleaseSpinlock(&dynamicAllocatorMutex);
            return NULL;
        }
        //TODO: this should be handled somehow
    }
    KeReleaseSpinlock(&dynamicAllocatorMutex);
    return (void*)(vAddress + (pAddress % MM_PAGE_SIZE));
}

void MmUnmapDynamicMemory(void *ptr, uintptr_t n)
{
    n = ALIGN_UP(n, MM_PAGE_SIZE);
    ptr = (void*)ALIGN_DOWN((uintptr_t)ptr, MM_PAGE_SIZE);

    MmUnmapMemoryEx((uintptr_t)ptr, n);

    KeAcquireSpinlock(&dynamicAllocatorMutex);

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
    KeReleaseSpinlock(&dynamicAllocatorMutex);
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
                if(OK != MmGetPhysicalPageAddress(kernelArgs->initrdAddress + (i * MM_PAGE_SIZE), &dummy))
                    KePanicEx(BOOT_FAILURE, MM_DYNAMIC_MEMORY_INIT_FAILURE, 0, 0, 0);
            }

            if(kernelArgs->initrdAddress - MM_DYNAMIC_START_ADDRESS)
            {
                if(NULL == MmAvlInsert(&MM_DYNAMIC_ADDRESS_TREE, &MM_DYNAMIC_SIZE_TREE, 
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
        if(NULL == MmAvlInsert(&MM_DYNAMIC_ADDRESS_TREE, &MM_DYNAMIC_SIZE_TREE, 
            kernelArgs->initrdAddress + kernelArgs->initrdSize, remaining))
                KePanicEx(BOOT_FAILURE, MM_DYNAMIC_MEMORY_INIT_FAILURE, OUT_OF_RESOURCES, 0, 0);
    }
}


