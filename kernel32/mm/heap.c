#include "heap.h"
#include "../mm/mm.h"
#include "../../drivers/vga/vga.h"

uint32_t kernelHeapTop = MM_KERNEL_HEAP_START; //current kernel heap top

struct HeapBlockMeta
{
    uint32_t size;
    struct HeapBlockMeta *next;
    uint8_t free;
};

struct HeapBlockMeta *heapBlockHead = NULL;

struct HeapBlockMeta *mm_expandHeapBlock(struct HeapBlockMeta *block, uint32_t n)
{
    if((MM_KERNEL_HEAP_START + MM_KERNEL_HEAP_MAX_SIZE - kernelHeapTop) < n) //check if there is enough memory on the heap
        return NULL;
    
    uint32_t pages = n / MM_PAGE_SIZE; //calculate number of required pages
    uint32_t remainder = MM_PAGE_SIZE - (n % MM_PAGE_SIZE); //calculate remainder to the next page boundary
    pages += (remainder > 0); //update page count if data length is not a multiple of a page size
    if(Mm_allocateEx(kernelHeapTop, pages, MM_PAGE_FLAG_WRITABLE) != OK) //try to allocate pages and map them to the kernel heap space
        return NULL;

    kernelHeapTop += n; //update heap top address
    
    //update block header
    block->next = NULL;
    block->size += n;

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
        block->next = nextBlock;
    }

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
                if(NULL != mm_expandHeapBlock(b, n - b->size))
                    break;
            }
        }
        //this block is not free, look for next block
        
        *previous = b; //store this block address
        b = b->next; //get next block address
    }
    //there is either a block metadata returned when found or NULL when there is no available block or the list is empty
    return b;
}


struct HeapBlockMeta *mm_newHeapBlock(struct HeapBlockMeta *previous, uint32_t n)
{
    n += sizeof(struct HeapBlockMeta); //update required size with metadata block size

    if((MM_KERNEL_HEAP_START + MM_KERNEL_HEAP_MAX_SIZE - kernelHeapTop) < n) //check if there is enough memory on the heap
        return NULL;
    
    uint32_t pages = n / MM_PAGE_SIZE; //calculate number of required pages
    uint32_t remainder = MM_PAGE_SIZE - (n % MM_PAGE_SIZE); //calculate remainder to the next page boundary
    pages += (remainder > 0); //update page count if data length is not a multiple of a page size
    if(Mm_allocateEx(kernelHeapTop, pages, MM_PAGE_FLAG_WRITABLE) != OK) //try to allocate pages and map them to the kernel heap space
        return NULL;

    struct HeapBlockMeta *block = (void*)kernelHeapTop; //get metadata block pointer
    kernelHeapTop += n; //update heap top address
    
    //fill new block header
    block->free = 0;
    block->next = NULL;
    block->size = n - sizeof(struct HeapBlockMeta);
    if(NULL != previous) //if there was some previous block, update its "next" pointer
        previous->next = block;

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
        block->next = nextBlock;
    }

    return block;
}

void *Mm_allocateKernelHeap(uint32_t n)
{
    if(n == 0)
        return NULL;
    
    struct HeapBlockMeta *block;

    if(NULL == heapBlockHead) //first call, there was nothing allocated before
    {
        block = mm_newHeapBlock(NULL, n); //allocate new heap block
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
            block = mm_newHeapBlock(previous, n);
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

    struct HeapBlockMeta *block = (struct HeapBlockMeta*)ptr - 1;
    block->free = 1; //mark block as free
    
    uint32_t freed = block->size; //how much memory is actually freed

    struct HeapBlockMeta *next = block->next; //get next block
    while(next && next->free) //loop for consecutive free blocks
    {
        freed += next->size + sizeof(struct HeapBlockMeta); //sum consecutive free memory
        block->next = next->next; //update (extend) first block
        next = next->next;
    }
    block->free = freed;
}