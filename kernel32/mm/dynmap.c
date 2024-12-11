#include "dynmap.h"
#include "heap.h"
#include "ke/core/panic.h"
#include "ke/core/mutex.h"
#include "hal/arch.h"
#include "hal/mm.h"

#if 1
#define BST_PROVIDE_ABSTRACTION //provide Tree... names for Bst...
#include "rtl/bst.h"
#endif

#define MM_DYNAMIC_MEMORY_SPLIT_THRESHOLD (PAGE_SIZE) //dynamic memory region split threshold when requested block is smaller

static struct TreeNode *MmDynamicMemoryTree[3] = {NULL, NULL, NULL};
#define MM_DYNAMIC_ADDRESS_FREE_TREE MmDynamicMemoryTree[0]
#define MM_DYNAMIC_SIZE_FREE_TREE MmDynamicMemoryTree[1]
#define MM_DYNAMIC_ADDRESS_USED_TREE MmDynamicMemoryTree[2]

static KeSpinlock MmDynamicAllocatorLock = KeSpinlockInitializer;

struct MmDynamicMemoryRegion
{
    TREENODE; /**< Tree node structure */
    const void *base; /**< Region base */
    size_t size; /**< Region size */
    struct MmDynamicMemoryRegion *buddy; /**< Region base <-> region size buddy node pointer */
};

/**
 * @brief Convenience macro to extract \a MmDynamicMemoryRegion from \a treeNode pointer
 * @param treeNode Tree node pointer
 * @return \a MmDynamicMemoryRegion pointer
 */
#define REGION(treeNode) ((struct MmDynamicMemoryRegion*)((treeNode)->main))

static struct MmDynamicMemoryRegion* MmDynamicInsertFreePair(const void *base, size_t size)
{
    struct MmDynamicMemoryRegion *b = NULL, *s = NULL;
    b = MmAllocateKernelHeap(sizeof(*b));
    if(NULL == b)
        return NULL;
    s = MmAllocateKernelHeap(sizeof(*s));
    if(NULL == s)
    {
        MmFreeKernelHeap(b);
        return NULL;
    }

    b->tree.key = (uintptr_t)base;
    b->base = base;
    b->size = size;
    b->buddy = s;
    b->tree.main = b;

    s->tree.key = size;
    s->base = base;
    s->size = size;
    s->buddy = b;
    s->tree.main = s;

    MM_DYNAMIC_ADDRESS_FREE_TREE = TreeInsert(MM_DYNAMIC_ADDRESS_FREE_TREE, &b->tree);
    MM_DYNAMIC_SIZE_FREE_TREE = TreeInsert(MM_DYNAMIC_SIZE_FREE_TREE, &s->tree);

    return b;
}

void *MmReserveDynamicMemory(uintptr_t n)
{
    n = ALIGN_UP(n, PAGE_SIZE);
    
    PRIO prio = KeAcquireSpinlock(&MmDynamicAllocatorLock);
    barrier();

    struct TreeNode *region = TreeFindGreaterOrEqual(MM_DYNAMIC_SIZE_FREE_TREE, n);
    if(NULL == region) //no fitting region available
    {
        KeReleaseSpinlock(&MmDynamicAllocatorLock, prio);
        return NULL;
    }
    //else region is available
    uintptr_t vAddress = (uintptr_t)REGION(region)->buddy->base; //store its address
    size_t remainingSize = REGION(region)->size - n; //calculate remaining size

    struct MmDynamicMemoryRegion *node = MmAllocateKernelHeap(sizeof(*node));
    if(NULL == node)
    {
        goto MmMapDynamicMemoryFail;  
    }

    node->tree.key = vAddress;
    node->tree.main = node;
    node->base = (void*)vAddress;
    node->size = REGION(region)->size;
    MM_DYNAMIC_ADDRESS_USED_TREE = TreeInsert(MM_DYNAMIC_ADDRESS_USED_TREE, &node->tree);

    //delete old nodes
    MM_DYNAMIC_ADDRESS_FREE_TREE = TreeRemove(MM_DYNAMIC_ADDRESS_FREE_TREE, &REGION(region)->buddy->tree);
    MM_DYNAMIC_SIZE_FREE_TREE = TreeRemove(MM_DYNAMIC_SIZE_FREE_TREE, region);

    MmFreeKernelHeap(REGION(region)->buddy);
    MmFreeKernelHeap(region);

    //check if it should be split
    if(remainingSize >= MM_DYNAMIC_MEMORY_SPLIT_THRESHOLD)
    {
        if(NULL != MmDynamicInsertFreePair((void*)(vAddress + n), remainingSize))
            node->size = n; //free pair insertion successful, reserved region is smaller
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
    struct TreeNode *region = TreeFindExact(MM_DYNAMIC_ADDRESS_USED_TREE, (uintptr_t)ptr);
    if(NULL == region)
    {
        KeReleaseSpinlock(&MmDynamicAllocatorLock, prio);
        return 0;
    }

    size_t n = REGION(region)->size;
    size_t originalSize = n;
    const void *originalPtr = ptr;
    MM_DYNAMIC_ADDRESS_USED_TREE = TreeRemove(MM_DYNAMIC_ADDRESS_USED_TREE, region);
    MmFreeKernelHeap(REGION(region));

    //check if there is an adjacent free region with higher address
    struct TreeNode *node = TreeFindExact(MM_DYNAMIC_ADDRESS_FREE_TREE, (uintptr_t)ptr + n);
    if(NULL != node)
    {
        n += REGION(node)->buddy->size; //sum size
        MM_DYNAMIC_SIZE_FREE_TREE = TreeRemove(MM_DYNAMIC_SIZE_FREE_TREE, &REGION(node)->buddy->tree);
        MM_DYNAMIC_ADDRESS_FREE_TREE = TreeRemove(MM_DYNAMIC_ADDRESS_FREE_TREE, node);
        MmFreeKernelHeap(REGION(node)->buddy);
        MmFreeKernelHeap(REGION(node));
    }
    //check if there is an adjacent preceeding free region
    node = TreeFindLess(MM_DYNAMIC_ADDRESS_FREE_TREE, (uintptr_t)ptr);
    if((NULL != node)
        && (((uintptr_t)REGION(node)->base + REGION(node)->buddy->size) == (uintptr_t)ptr))
    {
        n += REGION(node)->buddy->size; //sum size
        ptr = REGION(node)->base; //store new address
        MM_DYNAMIC_SIZE_FREE_TREE = TreeRemove(MM_DYNAMIC_SIZE_FREE_TREE, &REGION(node)->buddy->tree);
        MM_DYNAMIC_ADDRESS_FREE_TREE = TreeRemove(MM_DYNAMIC_ADDRESS_FREE_TREE, &REGION(node)->tree);
        MmFreeKernelHeap(REGION(node)->buddy);
        MmFreeKernelHeap(REGION(node));
    }

    if(NULL == MmDynamicInsertFreePair(ptr, n))
        LOG(SYSLOG_ERROR, "Unable to insert dynamic memory free region node. Memory leak!");

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
    if(NULL == MmDynamicInsertFreePair((void*)HalGetDynamicSpaceBase(), HalGetDynamicSpaceSize()))
        KePanicEx(BOOT_FAILURE, 0, OUT_OF_RESOURCES, 0, 0);
}


