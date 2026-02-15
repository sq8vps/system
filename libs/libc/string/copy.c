#include "string.h"
#include "stdlib.h"

void *memcpy(void * restrict s1, const void * restrict s2, size_t n)
{
    unsigned char *b1 = s1;
    unsigned const char *b2 = s2;
    while(n--)
    {
        *b1++ = *b2++;
    }
    return s1;
}

void *memccpy(void * restrict s1, const void * restrict s2, int c, size_t n)
{
    unsigned char *b1 = s1;
    unsigned const char *b2 = s2;
    while(n--)
    {
        *b1++ = *b2++;
        if(*(b2 - 1) == (unsigned char)c)
        {
            return b1;
        }
    }
    return NULL;
}

void *memmove(void *s1, const void *s2, size_t n)
{
    char *c1 = s1;
    const char *c2 = s2;
    if(c1 > c2)
    {
        while(n--)
            c1[n] = c2[n];
    }
    else if(c2 > c1)
    {
        for(size_t i = 0; i < n; ++i)
            c1[i] = c2[i];
    }
    
    return s1;
}

char *strcpy(char * restrict s1, const char * restrict s2)
{
    char *ret = s1;
    while(0 != (*s1++ = *s2++))
        ;
    return ret;
}

char *strncpy(char * restrict s1, const char * restrict s2, size_t n)
{
    char *ret = s1;
    while(('\0' != *s2) && (0 != n))
    {
        *s1++ = *s2++;
        --n;
    }
    while(n--)
    {
        *s1++ = '\0';
    }
    return ret;
}

char *strdup(const char *s)
{
    char *str = malloc(strlen(s) + 1);
    if(NULL == str)
        return NULL;
    strcpy(str, s);
    return str;
}

char *strndup(const char *s, size_t size)
{
    size_t length = strlen(s);
    if(length > size)
        length = size;

    char *str = malloc(length + 1);
    if(NULL == str)
        return NULL;
    memcpy(str, s, length + 1);
    return str;
}