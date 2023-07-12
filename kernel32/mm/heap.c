#include "heap.h"
#include "../mm/mm.h"
#include <stdbool.h>
#include "../ke/mutex.h"

#define MM_KERNEL_HEAP_START 0xD8000000 //kernel heap start address
#define MM_KERNEL_HEAP_MAX_SIZE 0x10000000 //kernel heap max size
#define MM_KERNEL_HEAP_ALIGNMENT 8 //required kernel heap block address alignment
#define MM_KERNEL_HEAP_DEALLOCATION_THRESHOLD 0x100000 //free heap memory deallocation threshold

uintptr_t kernelHeapTop = MM_KERNEL_HEAP_START; //current kernel heap top

/**
 * @brief A heap block metadata (header) structure
*/
struct HeapBlockMeta
{
    uintptr_t size; //size of usable block (not including the size of this structure)
    struct HeapBlockMeta *next; //next block in the list
    struct HeapBlockMeta *previous; //previous block in the list
    uint8_t free; //is this block free?
};
#define META_SIZE ALIGN_VAL(sizeof(struct HeapBlockMeta), MM_KERNEL_HEAP_ALIGNMENT)

/**
 * @brief Block metadata linked list head
*/
struct HeapBlockMeta *heapBlockHead = NULL;

static KeSpinLock_t heapAllocatorMutex;

/**
 * @brief Create new or extend exisiting heap block
 * @param previous Previous block pointer
 * @param n Count of bytes to allocate
 * @param expand 0 - new block creation mode, 1 - extension mode
*/
static struct HeapBlockMeta *mmAllocateHeapBlock(struct HeapBlockMeta *previous, uint32_t n, bool expand)
{
    if(!expand) //new block allocation mode
        n += META_SIZE; //update required size with metadata block size
    
    KeAcquireSpinlockDisableIRQ(&heapAllocatorMutex);

    if((MM_KERNEL_HEAP_START + MM_KERNEL_HEAP_MAX_SIZE - kernelHeapTop) < n) //check if there is enough memory on the heap
    {
        KeReleaseSpinlockEnableIRQ(&heapAllocatorMutex);
        return NULL;
    }
    
    uint32_t remainder = MM_PAGE_SIZE - (n % MM_PAGE_SIZE); //calculate remainder to the next page boundary

    if(OK != MmAllocateMemory(kernelHeapTop, n + remainder, MM_PAGE_FLAG_WRITABLE)) //try to allocate pages and map them to kernel heap space
    {
        KeReleaseSpinlockEnableIRQ(&heapAllocatorMutex);
        return NULL;
    }

    struct HeapBlockMeta *block = (struct HeapBlockMeta*)kernelHeapTop; //get metadata block pointer
    
    if(!expand) //new block allocation mode
    {
        //fill new block header
        block->free = 0;
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

    kernelHeapTop += n; //update heap top address

    /*
    * There is the problem that the low-level memory allocator allocates only multiples of page size
    * When block size is not a multiple of page size the remainder should be stored as a free block
    * to avoid memory waste.
    * Anyway drop it if remaining space is smaller than metadata size
    */
    if(remainder >= META_SIZE)
    {
        struct HeapBlockMeta *nextBlock = (void*)kernelHeapTop; //get metadata block pointer
        nextBlock->free = 1;
        nextBlock->size = remainder - META_SIZE;
        nextBlock->next = NULL;
        kernelHeapTop += remainder;
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

    KeReleaseSpinlockEnableIRQ(&heapAllocatorMutex);

    if(!expand) //new block allocation mode
        return block;
    
    return previous;
}

/**
 * @brief Split existing heap block
 * @param block Block to be split. Must be free
 * @param n Count of bytes required
 * @return Block of size n
 * 
 * 
 * This function takes an existing block of size N and a number n,
 * checks if splitting this block makes sense, resizes exisiting block to n bytes
 * and creates new block header of size N-n-sizeof(header) and adds it to the list.
*/
static struct HeapBlockMeta *mmSplitHeapBlock(struct HeapBlockMeta *block, uint32_t n)
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
    struct HeapBlockMeta *nextBlock = (struct HeapBlockMeta*)((uintptr_t)(block + 1) + n);
    block->size = n; //resize exisiting block
    nextBlock->free = 1;
    nextBlock->size = remainder - META_SIZE;
    nextBlock->next = block->next; //update list pointers
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
static struct HeapBlockMeta *mmFindPreallocatedHeapBlock(struct HeapBlockMeta **previous, uint32_t n)
{
    struct HeapBlockMeta *b = heapBlockHead; //start from head

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
        //this block is not free, look for next block
        
        *previous = b; //store this block address
        b = b->next; //get next block address
    }
    //there is either a block metadata returned when found or NULL when there is no available block or the list is empty
    return mmSplitHeapBlock(b, n); //split block if it's too big
}

void *MmAllocateKernelHeap(uintptr_t n)
{
    if(n == 0)
        return NULL;
    
    struct HeapBlockMeta *block;

    n = ALIGN_VAL(n, MM_KERNEL_HEAP_ALIGNMENT);

    KeAcquireSpinlockDisableIRQ(&heapAllocatorMutex);
    
    if(NULL == heapBlockHead) //first call, there was nothing allocated before
    {
        block = mmAllocateHeapBlock(NULL, n, 0); //allocate new heap block
        if(NULL == block)
        {
            KeReleaseSpinlockEnableIRQ(&heapAllocatorMutex);
            return NULL;
        }
        
        heapBlockHead = block; //update head
    }
    else //not the first call
    {
        struct HeapBlockMeta *previous = heapBlockHead;

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
                KeReleaseSpinlockEnableIRQ(&heapAllocatorMutex);
                return NULL;
            }
        }

    }
    KeReleaseSpinlockEnableIRQ(&heapAllocatorMutex);
    return (void*)((uintptr_t)block + META_SIZE);
}

void MmFreeKernelHeap(const void *ptr)
{
    if((uintptr_t)ptr < MM_KERNEL_HEAP_START || ((uintptr_t)ptr > (MM_KERNEL_HEAP_START + MM_KERNEL_HEAP_MAX_SIZE - META_SIZE)))
        return;

    struct HeapBlockMeta *block = (struct HeapBlockMeta*)((uintptr_t)ptr - META_SIZE); //get block header

    KeAcquireSpinlockDisableIRQ(&heapAllocatorMutex);

    block->free = 1; //mark block as free
    
    struct HeapBlockMeta *first = block; //first consecutive free block in the list
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
        block->previous = first;
    }
    first->next = block->next;
    first->size = freed;
    KeReleaseSpinlockEnableIRQ(&heapAllocatorMutex);
}