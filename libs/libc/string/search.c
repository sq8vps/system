#include "string.h"

void *memchr(void *s, int c, size_t n)
{
    unsigned char *k = s;
    while(n--)
    {
        if(*k == (unsigned char)c)
            return k;
        ++k;
    }
    return NULL;
}

char *strchr(char *s, int c)
{
    while(1)
    {
        if(*s == (char)c)
            return s;
        if('\0' == *s)
            return NULL;
        ++s;
    }
}

size_t strcspn(const char *s1, const char *s2)
{
    const char *initial = s1;
    while('\0' != *s1)
    {
        const char *c = s2;
        while('\0' != *c)
        {
            if(*s1 == *c)
                return s1 - initial;
            ++c;
        }
        ++s1;
    }
    return (size_t)(s1 - initial);
}

char *strpbrk(char *s1, const char *s2)
{
    while('\0' != *s1)
    {
        const char *c = s2;
        while('\0' != *c)
        {
            if(*s1 == *c)
                return s1;
            ++c;
        }
        ++s1;
    }
    return NULL;
}

char *strrchr(char *s, int c)
{
    char *base = s;
    s += strlen(s) + 1;
    while(*s != (char)c)
    {
        if(s == base)
            return NULL;
        --s;
    }
    return s;
}

size_t strspn(const char *s1, const char *s2)
{
    const char *initial = s1;
    while('\0' != *s1)
    {
        const char *c = s2;
        while('\0' != *c)
        {
            if(*s1 == *c)
                break;
            ++c;
        }
        if('\0' == *c)
            break;
        ++s1;
    }
    return (size_t)(s1 - initial);
}

char *strstr(char *s1, const char *s2)
{
    char *end = s1 + strlen(s1);
    size_t size = strlen(s2);
    if(0 == size)
        return s1;

    while(size < (size_t)(end - s1))
    {
        if(0 == memcmp(s1, s2, size))
            return s1;
        ++s1;
    }
    return NULL;
}

char *strtok(char * restrict s1, const char * restrict s2)
{
    static char *next = NULL;
    char *token = NULL;

    if(NULL == s1)
        s1 = next;

    s1 += strspn(s1, s2);
    if('\0' == *s1)
    {
        next = NULL;
        return NULL;
    }

    token = s1;
    s1 = strpbrk(token, s2);
    if(NULL == s1)
    {
        next = NULL;
    }
    else
    {
        *s1 = '\0';
        next = s1 + 1;
    }
    return token;
}