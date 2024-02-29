#include "heap.h"
#include "mm/mm.h"
#include <stdbool.h>
#include "ke/core/mutex.h"
#include "common.h"
#include "avl.h"

#define MM_KERNEL_HEAP_START 0xD8000000 //kernel heap start address
#define MM_KERNEL_HEAP_MAX_SIZE 0x10000000 //kernel heap max size
#define MM_KERNEL_HEAP_ALIGNMENT 8 //required kernel heap block address alignment
#define MM_KERNEL_HEAP_DEALLOCATION_THRESHOLD 0x100000 //free heap memory deallocation threshold

static uintptr_t MmKernelHeapTop = MM_KERNEL_HEAP_START; //current kernel heap top

/**
 * @brief A heap block metadata (header) structure
*/
struct MmHeapBlock
{
    bool free; //is block free?
    uintptr_t size; //block size, not including structure size
    //using linked list, the freeing is fast (we get the address and the structure is at this address)
    //however, allocation might be very slow if memory is fragmented
    //use AVL tree to store free blocks to reduce allocation time
    struct MmAvlNode avl;
    //pointer to neighboring blocks, either free or not
    struct MmHeapBlock *previous;
    struct MmHeapBlock *next;
};

#define META_SIZE ALIGN_UP(sizeof(struct MmHeapBlock), MM_KERNEL_HEAP_ALIGNMENT)

/**
 * @brief Block metadata linked list head
*/
static struct MmHeapBlock *MmHeapBlockHead = NULL;

/**
 * @brief Free block AVL tree
*/
static struct MmAvlNode *MmFreeHeapBlockTree = NULL;


/**
 * @brief Create new or extend exisiting heap block
 * @param previous Previous block pointer
 * @param n Count of bytes to allocate
 * @param expand 0 - new block creation mode, 1 - extension mode
*/
static struct MmHeapBlock *mmAllocateHeapBlock(struct MmHeapBlock *previous, uint32_t n, bool expand)
{
    if(!expand) //new block allocation mode
        n += META_SIZE; //update required size with metadata block size

    if((MM_KERNEL_HEAP_START + MM_KERNEL_HEAP_MAX_SIZE - MmKernelHeapTop) < n) //check if there is enough memory on the heap
    {
        return NULL;
    }
    
    uint32_t remainder = MM_PAGE_SIZE - (n % MM_PAGE_SIZE); //calculate remainder to the next page boundary

    if(OK != MmAllocateMemory(MmKernelHeapTop, n + remainder, MM_PAGE_FLAG_WRITABLE)) //try to allocate pages and map them to kernel heap space
    {
        return NULL;
    }

    struct MmHeapBlock *block = (struct MmHeapBlock*)MmKernelHeapTop; //get metadata block pointer
    
    if(!expand) //new block allocation mode
    {
        //fill new block header
        block->free = false;
        block->next = NULL;
        block->previous = previous;
        block->size = n - META_SIZE;
        if(NULL != previous) //if there was some previous block, update its "next" pointer
        {
            previous->next = block;
        }
    }
    else //block expansion mode
    {
        //update block header
        previous->size += n;
    }

    MmKernelHeapTop += n; //update heap top address

    /*
    * There is the problem that the low-level memory allocator allocates only multiples of page size
    * When block size is not a multiple of page size the remainder should be stored as a free block
    * to avoid memory waste.
    * Anyway drop it if remaining space is smaller than metadata size
    */
    if(remainder >= META_SIZE)
    {
        struct MmHeapBlock *nextBlock = (void*)MmKernelHeapTop; //get metadata block pointer
        nextBlock->free = true;
        nextBlock->size = remainder - META_SIZE;
        nextBlock->next = NULL;
        MmKernelHeapTop += remainder;
        if(!expand) //new block allocation mode
        {
            block->next = nextBlock;
            nextBlock->previous = block;
        }
        else //block expansion mode
        {
            previous->next = nextBlock;
            nextBlock->previous = previous;
        }
    }

    if(!expand) //new block allocation mode
        return block;
    
    return previous;
}

/**
 * @brief Split existing heap block
 * 
 * This function takes an existing block of size N and a number n,
 * checks if splitting this block makes sense, resizes exisiting block to n bytes
 * and creates new block header of size N-n-sizeof(header) and adds it to the list.
 * @param block Block to be split. Must be free
 * @param n Count of bytes required
 * @return Block of size n
*/
static struct MmHeapBlock *mmSplitHeapBlock(struct MmHeapBlock *block, uint32_t n)
{
    if(NULL == block)
        return NULL;

    if(!block->free)
        return NULL;

    if(block->size < n)
        return NULL;

    uint32_t remainder = block->size - n; //how many bytes are left after resizing the block
    if(remainder <= META_SIZE)
        return block; //only the header and 1 byte would fit, no point in splitting. Return original block
     
    //create new block after the block being resized
    struct MmHeapBlock *nextBlock = (struct MmHeapBlock*)((uintptr_t)(block + 1) + n);
    block->size = n; //resize exisiting block
    nextBlock->free = 1;
    nextBlock->size = remainder - META_SIZE;
    nextBlock->next = block->next; //update list pointers
    if(NULL != block->next)
        block->next->previous = nextBlock;
    nextBlock->previous = block;
    block->next = nextBlock;

    return block;
}

/**
 * @brief Find first free and matching block that was allocated before
 * @param previous Previous block handle will be returned here
 * @param n Block size (in bytes)
 * @return Block metadata or NULL if no previously allocated block available
*/
static struct MmHeapBlock *mmFindPreallocatedHeapBlock(struct MmHeapBlock **previous, uint32_t n)
{
    struct MmHeapBlock *b = MmHeapBlockHead; //start from head

    //run down the list
    while(b)
    {
        if(b->free) //is this block free?
        {
            if(b->size >= n) //is it big enough?
                break;
            
            if(NULL == b->next) //if block is free and it is the last block, even if it's too small, it can be expanded
            {
                return mmAllocateHeapBlock(b, n - b->size, 1);
            }
        }
        //this block is not free, look for the next block
        
        *previous = b; //store this block address
        b = b->next; //get next block address
    }
    //there is either a block metadata returned when found or NULL when there is no available block or the list is empty
    return mmSplitHeapBlock(b, n); //split block if it's too big
}

static KeSpinlock heapAllocatorMutex = KeSpinlockInitializer;

void *MmAllocateKernelHeap(uintptr_t n)
{
    if(n == 0)
        return NULL;
    
    struct MmHeapBlock *block;

    n = ALIGN_UP(n, MM_KERNEL_HEAP_ALIGNMENT);

    KeAcquireSpinlock(&heapAllocatorMutex);
    
    if(NULL == MmHeapBlockHead) //first call, there was nothing allocated before
    {
        block = mmAllocateHeapBlock(NULL, n, 0); //allocate new heap block
        if(NULL == block)
        {
            KeReleaseSpinlock(&heapAllocatorMutex);
            return NULL;
        }
        
        MmHeapBlockHead = block; //update head
    }
    else //not the first call
    {
        struct MmHeapBlock *previous = MmHeapBlockHead;

        block = mmFindPreallocatedHeapBlock(&previous, n); //look if there is any preallocated block available
        if(block) //found free block?
        {
            block->free = 0;

        }
        else //not found?
        {
            block = mmAllocateHeapBlock(previous, n, 0);
            if(NULL == block) //failure
            {
                KeReleaseSpinlock(&heapAllocatorMutex);
                return NULL;
            }
        }

    }
    KeReleaseSpinlock(&heapAllocatorMutex);
    return (void*)((uintptr_t)block + META_SIZE);
}



void MmFreeKernelHeap(const void *ptr)
{
    if((uintptr_t)ptr < (MM_KERNEL_HEAP_START + META_SIZE) || ((uintptr_t)ptr > (MM_KERNEL_HEAP_START + MM_KERNEL_HEAP_MAX_SIZE - META_SIZE)))
        return;

    struct MmHeapBlock *block = (struct MmHeapBlock*)((uintptr_t)ptr - META_SIZE); //get block header

    KeAcquireSpinlock(&heapAllocatorMutex);

    block->free = 1; //mark block as free
    
    struct MmHeapBlock *first = block; //first consecutive free block in the list
    while((NULL != first->previous) && first->previous->free) //find the very first consecutive free block
    {
       first = first->previous;
    }
    //then start from this first block

    block = first; //now "block" pointer will be used
    uint32_t freed = block->size; //how much memory is actually freed
    while((NULL != block->next) && block->next->free) //loop for next consecutive free blocks
    {
        block = block->next;
        freed += block->size + META_SIZE; //sum consecutive free memory
        first->next = block; //update (extend) first block
    }
    first->next = block->next;
    if(NULL != block->next)
        block->next->previous = first;
    first->size = freed;
    KeReleaseSpinlock(&heapAllocatorMutex);
}

void *MmAllocateKernelHeapZeroed(uintptr_t n)
{
    void *p = MmAllocateKernelHeap(n);
    if(NULL != p)
        CmMemset(p, 0, n);
    return p;
}

// void *MmReallocateKernelHeap(void *ptr, uintptr_t n)
// {
//     if(0 == n)
//     {
//         MmFreeKernelHeap(ptr);
//         return NULL;
//     }
//     void *m = MmAllocateKernelHeap(n);
//     if(NULL == m)
//         return NULL;
//     if(NULL == ptr)
//         return m;
//     struct MmHeapBlock *block = (struct MmHeapBlock*)((uintptr_t)ptr - META_SIZE);
//     CmMemcpy(m, ptr, block->size);
//     return m;
// }

#if MM_KERNEL_HEAP_START & (MM_KERNEL_HEAP_ALIGNMENT - 1)
    #error Kernel heap start address is not aligned
#endif

#if (MM_KERNEL_HEAP_ALIGNMENT & 0xF) || (MM_KERNEL_HEAP_ALIGNMENT == 0)
    #error Kernel heap must be aligned to a non-zero multiple of 16 bytes
#endif