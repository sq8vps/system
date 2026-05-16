#include "string.h"

int memcmp(const void *s1, const void *s2, size_t n)
{
    const unsigned char *c1 = s1, *c2 = s2;
    while(n--)
    {
        if(*c1 != *c2)
        {
            return *c1 - *c2;
        }
        ++c1;
        ++c2;
    }
    return 0;
}

int strcmp(const char *s1, const char *s2)
{
    while(*s1 && (*s1 == *s2))
    {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

int strcoll(const char *s1, const char *s2)
{
    return strcmp(s1, s2);
}

int strncmp(const char *s1, const char *s2, size_t n)
{
    while(*s1 && (*s1 == *s2) && n--)
    {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

size_t strxfrm(char * restrict s1, const char * restrict s2, size_t n)
{
    //TODO: implement locales
    strncpy(s1, s2, n);
    return strlen(s1);
}