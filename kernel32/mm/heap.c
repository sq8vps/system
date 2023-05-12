#include "heap.h"
#include "../mm/mm.h"
#include <stdbool.h>

uint32_t kernelHeapTop = MM_KERNEL_HEAP_START; //current kernel heap top

/**
 * @brief A heap block metadata (header) structure
*/
struct HeapBlockMeta
{
    uint32_t size; //size of usable block (not including the size of this structure)
    struct HeapBlockMeta *next; //next block in the list
    struct HeapBlockMeta *previous; //previous block in the list
    uint8_t free; //is this block free?
};

/**
 * @brief Block metadata linked list head
*/
struct HeapBlockMeta *heapBlockHead = NULL;


/**
 * @brief Create new or extend exisiting heap block
 * @param previous Previous block pointer
 * @param n Count of bytes to allocate
 * @param expand 0 - new block creation mode, 1 - extension mode
*/
struct HeapBlockMeta *mm_allocateHeapBlock(struct HeapBlockMeta *previous, uint32_t n, bool expand)
{
    if(!expand) //new block allocation mode
        n += sizeof(struct HeapBlockMeta); //update required size with metadata block size

    if((MM_KERNEL_HEAP_START + MM_KERNEL_HEAP_MAX_SIZE - kernelHeapTop) < n) //check if there is enough memory on the heap
        return NULL;
    
    uint32_t pages = n / MM_PAGE_SIZE; //calculate number of required pages
    uint32_t remainder = MM_PAGE_SIZE - (n % MM_PAGE_SIZE); //calculate remainder to the next page boundary
    pages += (remainder > 0); //update page count if data length is not a multiple of a page size
    if(Mm_allocateEx(kernelHeapTop, pages, MM_PAGE_FLAG_WRITABLE) != OK) //try to allocate pages and map them to the kernel heap space
        return NULL;

    struct HeapBlockMeta *block = (void*)kernelHeapTop; //get metadata block pointer
    
    if(!expand) //new block allocation mode
    {
        //fill new block header
        block->free = 0;
        block->next = NULL;
        block->previous = previous;
        block->size = n - sizeof(struct HeapBlockMeta);
        if(NULL != previous) //if there was some previous block, update its "next" pointer
            previous->next = block;
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
    if(remainder >= sizeof(struct HeapBlockMeta))
    {
        struct HeapBlockMeta *nextBlock = (void*)kernelHeapTop; //get metadata block pointer
        nextBlock->free = 1;
        nextBlock->size = remainder - sizeof(struct HeapBlockMeta);
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
 * This function takes an existing block of size N and a number n,
 * checks if splitting this block makes sense, resizes exisiting block to n bytes
 * and creates new block header of size N-n-sizeof(header) and adds it to the list.
*/
struct HeapBlockMeta *mm_splitHeapBlock(struct HeapBlockMeta *block, uint32_t n)
{
    if(NULL == block)
        return NULL;

    uint32_t remainder = block->size - n; //how much bytes are left after resizing the block
    if(remainder <= sizeof(struct HeapBlockMeta))
        return block; //only the header and 1 byte would fit, no point in splitting. Return original block
    
    block->size = n; //resize exisitng block
    
    //create new block after the block being resized
    struct HeapBlockMeta *nextBlock = (struct HeapBlockMeta*)((uint32_t)(block + 1) + n) ;
    nextBlock->free = 1;
    nextBlock->size = remainder - sizeof(struct HeapBlockMeta);
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
struct HeapBlockMeta *mm_findPreallocatedHeapBlock(struct HeapBlockMeta **previous, uint32_t n)
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
                return mm_allocateHeapBlock(b, n - b->size, 1);
            }
        }
        //this block is not free, look for next block
        
        *previous = b; //store this block address
        b = b->next; //get next block address
    }
    //there is either a block metadata returned when found or NULL when there is no available block or the list is empty
    return mm_splitHeapBlock(b, n); //split block if it's too big
}


void *Mm_allocateKernelHeap(uint32_t n)
{
    if(n == 0)
        return NULL;
    
    struct HeapBlockMeta *block;

    if(NULL == heapBlockHead) //first call, there was nothing allocated before
    {
        block = mm_allocateHeapBlock(NULL, n, 0); //allocate new heap block
        if(NULL == block) //failre
            return NULL;
        
        heapBlockHead = block; //update head
    }
    else //not the first call
    {
        struct HeapBlockMeta *previous = heapBlockHead;
        block = mm_findPreallocatedHeapBlock(&previous, n); //look if there is any preallocated block available
        if(block) //found free block?
        {
            block->free = 0;
        }
        else //not found?
        {
            block = mm_allocateHeapBlock(previous, n, 0);
            if(NULL == block) //failure
                return NULL;
        }
    }
    return (block + 1);
}

void Mm_freeKernelHeap(void *ptr)
{
    if((uint32_t)ptr < MM_KERNEL_HEAP_START || ((uint32_t)ptr > (MM_KERNEL_HEAP_START + MM_KERNEL_HEAP_MAX_SIZE - sizeof(struct HeapBlockMeta))))
        return;

    struct HeapBlockMeta *block = (struct HeapBlockMeta*)ptr - 1; //get block header
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
        freed += block->size + sizeof(struct HeapBlockMeta); //sum consecutive free memory
        first->next = block; //update (extend) first block
        block->previous = first;
    }
    first->size = freed;
}