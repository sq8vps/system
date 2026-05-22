#ifndef NABLA_LIBC_NABLA_STATE_H
#define NABLA_LIBC_NABLA_STATE_H

#include <stddef.h>

struct _nabla_libc_thread_state
{
    struct _nabla_libc_thread_state *self;
    void *heap_state;
    int errno;
    struct
    {
        size_t page_size;
    } config;
};

#endif