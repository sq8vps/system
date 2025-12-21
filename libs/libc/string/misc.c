#include "string.h"
#include "nabla/utils.h"

void *memset(void *s, int c, size_t n)
{
    unsigned char *sc = s;
    while(n--)
    {
        *sc = (unsigned char)c;
        ++sc;
    }
    return s;
}

void *memset_explicit(void *s, int c, size_t n)
{
    #pragma GCC push_options
    #pragma GCC optimize ("O0")
    unsigned char *sc = s;
    while(n--)
    {
        *sc = (unsigned char)c;
        ++sc;
    }
    return s;
    #pragma GCC pop_options
}

char *strerror(int errnum)
{
    return __nabla_libc_strerror(errnum);
}

size_t strlen(const char *s)
{
    const char *first = s;
    while('\0' != *s++)
        ;

    return s - first;
}