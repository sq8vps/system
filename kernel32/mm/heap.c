#include "heap.h"
#include "mm/mm.h"
#include <stdbool.h>
#include "ke/core/mutex.h"
#include "common.h"
#include "assert.h"

#define MM_KERNEL_HEAP_START 0xD8000000                // kernel heap start address
#define MM_KERNEL_HEAP_MAX_SIZE 0x10000000             // kernel heap max size
#define MM_KERNEL_HEAP_ALIGNMENT 16                    // required kernel heap block address alignment
#define MM_KERNEL_HEAP_DEALLOCATION_THRESHOLD 0x100000 // free heap memory deallocation threshold

/**
 * @brief A heap block metadata (header) structure
 */
struct MmHeapBlock
{
    bool free;      // is block free?
    uintptr_t size; // block size, not including structure size
    // pointer to neighboring blocks, either free or not
    struct MmHeapBlock *previous;
    struct MmHeapBlock *next;
};
#define META_SIZE ALIGN_UP(sizeof(struct MmHeapBlock), MM_KERNEL_HEAP_ALIGNMENT)

/**
 * @brief Block metadata linked list head
 */
static struct MmHeapBlock *MmHeapBlockHead = NULL;
static struct MmHeapBlock *MmHeapBlockTail = NULL;

#ifdef USE_AVL_TREES

/**
 * @brief Free block AVL tree
 */
static struct MmAvlNode *MmFreeHeapBlockTree = NULL;

static void mmInsertFreeHeapBlock(struct MmHeapBlock *block)
{
    block->avl.dynamic = false;
    block->avl.buddy = NULL;
    block->avl.left = NULL;
    block->avl.right = NULL;
    block->avl.height = 1;
    block->avl.ptr = block;
    block->avl.key = block->size;
    MmFreeHeapBlockTree = MmAvlInsertExisting(MmFreeHeapBlockTree, &(block->avl));
}
#endif

static struct MmHeapBlock *MmHeapAllocateNewBlock(uintptr_t n, uintptr_t align)
{
    uintptr_t heapTop = MM_KERNEL_HEAP_START;
    if (NULL != MmHeapBlockTail)
    {
        heapTop = (uintptr_t)MmHeapBlockTail + MmHeapBlockTail->size + META_SIZE;
    }
    uintptr_t alignedStart = ALIGN_UP(heapTop + META_SIZE, align);
    uintptr_t padding = alignedStart - (heapTop + META_SIZE);

    uintptr_t bytesToAllocate = ALIGN_UP(n + META_SIZE + padding, 4096);
    uintptr_t pageRemainder = bytesToAllocate - (n + META_SIZE + padding);

    if (bytesToAllocate > (MM_KERNEL_HEAP_START + MM_KERNEL_HEAP_MAX_SIZE - heapTop))
        return NULL;

    if (OK != MmAllocateMemory(heapTop, bytesToAllocate, 0))
        return NULL;

    struct MmHeapBlock *block = (struct MmHeapBlock *)(alignedStart - META_SIZE);
    block->next = NULL;
    block->free = false;

    if (NULL != MmHeapBlockTail)
    {
        MmHeapBlockTail->size += padding;
        MmHeapBlockTail->next = block;

        block->previous = MmHeapBlockTail;
        if (MmHeapBlockHead == MmHeapBlockTail)
            MmHeapBlockHead = block;
        MmHeapBlockTail = block;
    }
    else
    {
        block->previous = NULL;
        MmHeapBlockTail = block;
        MmHeapBlockHead = block;
    }

    if (pageRemainder >= (META_SIZE + MM_KERNEL_HEAP_ALIGNMENT))
    {
        block->size = n;

        struct MmHeapBlock *next = (struct MmHeapBlock *)((uintptr_t)block + n + META_SIZE);
        next->free = true;
        next->size = pageRemainder - META_SIZE;
        next->previous = block;
        next->next = NULL;
        MmHeapBlockTail = next;
        block->next = next;
    }
    else
        block->size = n + pageRemainder;

    return block;
}

static struct MmHeapBlock *MmSplitBlock(struct MmHeapBlock *block, uintptr_t n, uintptr_t align)
{
    if (NULL == block->previous)
    {
        if (((uintptr_t)block + META_SIZE) & (align - 1))
        {
            // this is the first block, but is not aligned correctly
            // in such case, a new block must be allocated, so return here
            return NULL;
        }
    }

    uintptr_t alignedStart = ALIGN_UP((uintptr_t)block + META_SIZE, align);
    uintptr_t padding = alignedStart - ((uintptr_t)block + META_SIZE);
    uintptr_t remaining = block->size - padding;

    if ((block->size - padding) < n)
        return NULL;

    struct MmHeapBlock original = *block;

    if (0 != padding)
    {
        // get new aligned block
        block = (struct MmHeapBlock *)(alignedStart - META_SIZE);
        *block = original;
        if (NULL != block->next)
            block->next->previous = block;
        block->previous->next = block;
        block->size -= padding;

        // resize previous block
        block->previous->size += padding;
    }

    if ((remaining - n) >= (META_SIZE + MM_KERNEL_HEAP_ALIGNMENT))
    {
        struct MmHeapBlock *newBlock = (struct MmHeapBlock *)((uintptr_t)block + META_SIZE + n);
        newBlock->free = true;
        newBlock->size = remaining - n - META_SIZE;
        newBlock->previous = block;
        newBlock->next = block->next;
        if (NULL == newBlock->next)
            MmHeapBlockTail = newBlock;
        else
            newBlock->next->previous = newBlock;
        block->next = newBlock;
        block->size = n;
    }
    else
    {
        if (NULL != block->next)
            MmHeapBlockTail = block;
        block->size = remaining;
    }

    block->free = false;
    return block;
}

static bool MmHeapExtendLastBlock(uintptr_t n)
{
    uintptr_t heapTop = (uintptr_t)MmHeapBlockTail + MmHeapBlockTail->size + META_SIZE;
    uintptr_t bytesToAllocate = ALIGN_UP(n - MmHeapBlockTail->size, 4096);

    if (bytesToAllocate > (MM_KERNEL_HEAP_START + MM_KERNEL_HEAP_MAX_SIZE - heapTop))
        return false;

    if (OK != MmAllocateMemory(heapTop, bytesToAllocate, 0))
        return false;

    MmHeapBlockTail->size += bytesToAllocate;
    return true;
}

void *MmAllocateKernelHeapAligned(uintptr_t n, uintptr_t align)
{
    struct MmHeapBlock *ret;

    n = ALIGN_UP(n, MM_KERNEL_HEAP_ALIGNMENT);

    if (align < MM_KERNEL_HEAP_ALIGNMENT)
        align = MM_KERNEL_HEAP_ALIGNMENT;

    if (1 != __builtin_popcountll(align))
        return NULL;

    if (NULL != MmHeapBlockHead)
    {
        struct MmHeapBlock *block = MmHeapBlockHead;
        while (block)
        {
            if (block->free)
            {
                if (block->size >= n)
                {
                    ret = MmSplitBlock(block, n, align);
                    if (NULL != ret)
                        return (void *)((uintptr_t)ret + META_SIZE);
                }
            }
            block = block->next;
        }

        // no block of proper size found
        if (MmHeapBlockTail->free)
        {
            uintptr_t padding = ALIGN_UP((uintptr_t)MmHeapBlockTail + META_SIZE, align) - ((uintptr_t)MmHeapBlockTail + META_SIZE);

            if (MmHeapExtendLastBlock(n + padding))
            {
                ret = MmSplitBlock(MmHeapBlockTail, n, align);
                if (NULL != ret)
                    return (void *)((uintptr_t)ret + META_SIZE);
            }
        }
    }

    ret = MmHeapAllocateNewBlock(n, align);
    if (NULL != ret)
        return (void *)((uintptr_t)ret + META_SIZE);
    else
        return NULL;
}

void *MmAllocateKernelHeap(uintptr_t n)
{
    return MmAllocateKernelHeapAligned(n, MM_KERNEL_HEAP_ALIGNMENT);
}

void *MmAllocateKernelHeapZeroed(uintptr_t n)
{
    void *ptr = MmAllocateKernelHeap(n);
    if (NULL != ptr)
        CmMemset(ptr, 0, n);

    return ptr;
}

void MmFreeKernelHeap(const void *ptr)
{
    if(NULL == ptr)
        return;
        
    struct MmHeapBlock *block = (struct MmHeapBlock *)((uintptr_t)ptr - META_SIZE);
    block->free = true;

    if ((NULL != block->next) && (block->next->free))
    {
        block->size += block->next->size + META_SIZE;
        block->next = block->next->next;
        if (NULL != block->next)
        {
            block->next->previous = block;
        }
        else
        {
            MmHeapBlockTail = block;
        }
    }

    if ((NULL != block->previous) && (block->previous->free))
    {
        block->previous->size += block->size + META_SIZE;
        block->previous->next = block->next;
        if (NULL == block->next)
        {
            MmHeapBlockTail = block->previous;
        }
        else
            block->next->previous = block->previous;
    }
}

#if MM_KERNEL_HEAP_START & (MM_KERNEL_HEAP_ALIGNMENT - 1)
#error Kernel heap start address is not aligned
#endif

#if (MM_KERNEL_HEAP_ALIGNMENT & 0x3) || (MM_KERNEL_HEAP_ALIGNMENT == 0)
#error Kernel heap must be aligned to a non-zero multiple of 8 bytes
#endif