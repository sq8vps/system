#include "stdlib.h"
#include "string.h"
#include "nabla/nabla.h"
#include "nabla/utils.h"
#include <stdint.h>
#include "errno.h"
#include "mm/tmem.h"
#include "ke/sys/syscall.h"

#define _NABLA_LIBC_MALLOC_INITIAL_POOL_SIZE 4 //number of native pages

struct _nabla_libc_malloc_meta
{
    bool free; //is block free?
    size_t size; //block size, not including structure size
    //pointer to neighboring blocks, either free or not
    struct _nabla_libc_malloc_meta *previous;
    struct _nabla_libc_malloc_meta *next;
};
#define MIN_ALLOCATION_ALIGNMENT 64
#define META_SIZE ALIGN_UP(sizeof(struct _nabla_libc_malloc_meta), MIN_ALLOCATION_ALIGNMENT)

struct _nabla_libc_malloc_state
{
    struct _nabla_libc_malloc_meta *head, *tail;
};


static struct _nabla_libc_malloc_meta *__allocate_block(size_t n, size_t alignment)
{
    STATUS status = OK;
    struct _nabla_libc_thread_state *tls = _nabla_get_tls();
    struct _nabla_libc_malloc_state *state = tls->heap_state;
    struct _nabla_libc_malloc_meta *block = NULL;
    size_t shift = 0;

    if(0 == alignment)
        alignment = 1;

    if(0 != (META_SIZE & (alignment - 1)))
        shift = (ALIGN_UP(META_SIZE, alignment) - META_SIZE);
    
    n = ALIGN_UP(n + META_SIZE + shift, MIN_ALLOCATION_ALIGNMENT);
    n = ALIGN_UP(n, tls->config.page_size);
    
    if(1 == alignment)
        status = ApiMapTaskMemoryA(NULL, n, MM_TASK_MEMORY_READABLE | MM_TASK_MEMORY_WRITABLE, (void**)&block);
    else
        status = ApiMapTaskMemory(NULL, n, MM_TASK_MEMORY_READABLE | MM_TASK_MEMORY_WRITABLE, -1, alignment, 0, 0, (void**)&block);
    
    if(OK != status)
        __LIBC_RETURN_STATUS(status, NULL);

    if(0 != shift)
    {
        block->free = true;
        block->size = shift - META_SIZE;
        if(NULL != state->tail)
        {
            state->tail->next = block;
            block->previous = state->tail;
            if(state->head == state->tail)
                state->head = block;
            state->tail = block;
        }
        else
        {
            block->previous = NULL;
            state->tail = block;
            state->head = block;
        }
    }

    block = (struct _nabla_libc_malloc_meta*)((uintptr_t)block + shift);

    block->next = NULL;
    block->free = false;

    if(NULL != state->tail)
    {
        state->tail->next = block;
        block->previous = state->tail;
        if (state->head == state->tail)
            state->head = block;
        state->tail = block;
    }
    else
    {
        block->previous = NULL;
        state->tail = block;
        state->head = block;
    }

    return block;
}

static struct _nabla_libc_malloc_meta *__split_block(struct _nabla_libc_malloc_meta *block, size_t n, size_t align)
{
    struct _nabla_libc_malloc_state *state = _nabla_get_tls()->heap_state;
    if (NULL == block->previous)
    {
        if(((uintptr_t)block + META_SIZE) & (align - 1))
        {
            // this is the first block, but is not aligned correctly
            // in such case, a new block must be allocated, so return here
            return NULL;
        }
    }

    uintptr_t alignedStart = ALIGN_UP((uintptr_t)block + META_SIZE, align);
    size_t padding = alignedStart - ((uintptr_t)block + META_SIZE);
    size_t remaining = block->size - padding;

    if ((block->size - padding) < n)
        return NULL;

    struct _nabla_libc_malloc_meta original = *block;

    if (0 != padding)
    {
        // get new aligned block
        block = (struct _nabla_libc_malloc_meta *)(alignedStart - META_SIZE);
        *block = original;
        if (NULL != block->next)
            block->next->previous = block;
        block->previous->next = block;
        block->size -= padding;

        // resize previous block
        block->previous->size += padding;
    }

    if ((remaining - n) >= (META_SIZE + MIN_ALLOCATION_ALIGNMENT))
    {
        struct _nabla_libc_malloc_meta *newBlock = (struct _nabla_libc_malloc_meta *)((uintptr_t)block + META_SIZE + n);
        newBlock->free = true;
        newBlock->size = remaining - n - META_SIZE;
        newBlock->previous = block;
        newBlock->next = block->next;
        if (NULL == newBlock->next)
            state->tail = newBlock;
        else
            newBlock->next->previous = newBlock;
        block->next = newBlock;
        block->size = n;
    }
    else
    {
        if (NULL == block->next)
            state->tail = block;
        block->size = remaining;
    }

    block->free = false;
    return block;
}

static bool __extend_last(size_t n)
{
    void *block;
    STATUS status = OK;
    struct _nabla_libc_thread_state *tls = _nabla_get_tls();
    struct _nabla_libc_malloc_state *state = tls->heap_state;
    uintptr_t heapTop = (uintptr_t)state->tail + state->tail->size + META_SIZE;
    size_t bytesToAllocate = ALIGN_UP(n - state->tail->size, tls->config.page_size);

    status = ApiMapTaskMemoryA((void*)heapTop, bytesToAllocate, MM_TASK_MEMORY_READABLE | MM_TASK_MEMORY_WRITABLE | MM_TASK_MEMORY_FIXED, &block);
    if(OK != status)
        return NULL;

    state->tail->size += bytesToAllocate;

    return true;
}

void *aligned_alloc(size_t alignment, size_t size)
{
    struct _nabla_libc_malloc_state *state = _nabla_get_tls()->heap_state;
    if(0 == size)
        return NULL;

    struct _nabla_libc_malloc_meta *ret;

    size = ALIGN_UP(size, MIN_ALLOCATION_ALIGNMENT);

    if(alignment < MIN_ALLOCATION_ALIGNMENT)
        alignment = MIN_ALLOCATION_ALIGNMENT;

    if(1 != __builtin_popcountll(alignment))
    {
        __LIBC_RETURN_ERRNO(-EALIGN, NULL);
    }

    if(NULL != state->head)
    {
        struct _nabla_libc_malloc_meta *block = state->head;
        while(block)
        {
            if(block->free)
            {
                if(block->size >= size)
                {
                    ret = __split_block(block, size, alignment);

                    if (NULL != ret)
                    {
                        return (void *)((uintptr_t)ret + META_SIZE);
                    }
                }
            }
            block = block->next;
        }

        //no block of proper size found
        if(state->tail->free)
        {
            size_t padding = ALIGN_UP((uintptr_t)state->tail + META_SIZE, alignment) - ((uintptr_t)state->tail + META_SIZE);

            if(__extend_last(size + padding))
            {
                ret = __split_block(state->tail, size, alignment);
                if (NULL != ret)
                {
                    return (void *)((uintptr_t)ret + META_SIZE);
                }
            }
        }
    }


    ret = __allocate_block(size, alignment);

    if(NULL != ret)
    {
        return (void *)((uintptr_t)ret + META_SIZE);
    }
    else
    {
        __LIBC_RETURN_ERRNO(-ENOMEM, NULL);
    }

}

void *malloc(size_t size)
{
    return aligned_alloc(MIN_ALLOCATION_ALIGNMENT, size);
}

void *calloc(size_t nmemb, size_t size)
{
    void *ptr = malloc(nmemb * size);
    if(NULL != ptr)
        memset(ptr, 0, nmemb * size);

    return ptr;
}

void free(void *ptr)
{
    struct _nabla_libc_malloc_state *state = _nabla_get_tls()->heap_state;
    if(NULL == ptr)
        return;
    
    struct _nabla_libc_malloc_meta *block = (struct _nabla_libc_malloc_meta *)((uintptr_t)ptr - META_SIZE);

    block->free = true;

    if((NULL != block->next) && (block->next->free))
    {
        block->size += block->next->size + META_SIZE;
        block->next = block->next->next;
        if (NULL != block->next)
        {
            block->next->previous = block;
        }
        else
        {
            state->tail = block;
        }
    }

    if((NULL != block->previous) && (block->previous->free))
    {
        block->previous->size += block->size + META_SIZE;
        block->previous->next = block->next;
        if (NULL == block->next)
        {
            state->tail = block->previous;
        }
        else
            block->next->previous = block->previous;
    }
}

void *realloc(void *ptr, size_t size)
{
    void *p = malloc(size);
    if(NULL == p)
        return NULL; //errno should be set by malloc()
    if(NULL == ptr)
        return p;
    
    memcpy(p, ptr, size);
    free(ptr);

    return p;
}
void free_sized(void *ptr, size_t size)
{
    (void)size;
    if(NULL == ptr)
        return;

    free(ptr);
}

void free_aligned_sized(void *ptr, size_t alignment, size_t size)
{
    (void)alignment;
    (void)size;
    if(NULL == ptr)
        return;

    free(ptr);
}

void *_nabla_create_tls(void)
{
    struct _nabla_libc_malloc_meta *tlsBlock = NULL, *stateBlock = NULL, *nextBlock = NULL;
    struct _nabla_libc_malloc_state *state = NULL;
    struct _nabla_libc_thread_state *tls = NULL;

    union
    {
        uint64_t u64;
        size_t s;
    } page_size;

    if(OK != ApiGetSystemConfig(API_SYSTEM_CONFIG_PAGE_SIZE, &(page_size.u64)))
        return NULL;

    if(OK != ApiMapTaskMemoryA(NULL, page_size.s * _NABLA_LIBC_MALLOC_INITIAL_POOL_SIZE, MM_TASK_MEMORY_READABLE | MM_TASK_MEMORY_WRITABLE, (void**)&stateBlock))
        return NULL;
    
    state = (struct _nabla_libc_malloc_state*)((uintptr_t)stateBlock + META_SIZE);
    stateBlock->free = false;
    stateBlock->size = ALIGN_UP(sizeof(struct _nabla_libc_malloc_state), MIN_ALLOCATION_ALIGNMENT);

    tlsBlock = (struct _nabla_libc_malloc_meta*)((uintptr_t)stateBlock + META_SIZE + stateBlock->size);
    tls = (struct _nabla_libc_thread_state*)((uintptr_t)tlsBlock + META_SIZE);
    tlsBlock->free = false;
    tlsBlock->size = ALIGN_UP(sizeof(struct _nabla_libc_thread_state), MIN_ALLOCATION_ALIGNMENT);

    nextBlock = (struct _nabla_libc_malloc_meta*)((uintptr_t)tlsBlock + META_SIZE + tlsBlock->size);
    nextBlock->free = true;
    nextBlock->size = (page_size.s * _NABLA_LIBC_MALLOC_INITIAL_POOL_SIZE) - ((uintptr_t)nextBlock - (uintptr_t)stateBlock - META_SIZE);

    stateBlock->previous = NULL;
    stateBlock->next = tlsBlock;
    tlsBlock->previous = stateBlock;
    tlsBlock->next = nextBlock;
    nextBlock->previous = tlsBlock;
    nextBlock->next = NULL;

    state->head = stateBlock;
    state->tail = nextBlock;

    memset(tls, 0, sizeof(*tls));
    tls->heap_state = state;
    tls->self = tls;

    return tls;
}