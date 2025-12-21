#ifndef NABLA_LIBC_NABLA_STATE_H
#define NABLA_LIBC_NABLA_STATE_H

struct _nabla_libc_thread_state
{
    void *heap_state;
    int errno;
};

#endif