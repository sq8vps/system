#include "sys/unistd.h"
#include "mm/tmem.h"
#include "errno.h"

void *_sbrk(ptrdiff_t __incr)
{
    void *r = ApiResizeHeap(__incr);
    if(nullptr == r)
    {
        errno = ENOMEM;
        return (void*)(-1);
    }
    return r;
}