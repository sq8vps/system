#include "common.h"

uint32_t Cm_strlen(const char *str)
{
    char *s = (char*)str;
    while(*s)
        s++;
    
    return (uintptr_t)s - (uintptr_t)str;
}

char* Cm_strcpy(char *strTo, const char *strFrom)
{
    char *initial = strTo;
    while(*strTo++ = *strFrom++);
    return initial;
}

int Cm_strcmp(const char *s1, const char *s2)
{
    while(*s1 && (*s1 == *s2))
    {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

void* Cm_memcpy(void *to, const void *from, uintptr_t n)
{
    void *initial = to;
    while(n--)
    {
        *((uint8_t*)to++) = *((uint8_t*)from++);
    }
    return initial;
}

uint32_t Cm_abs(int32_t x)
{
    return (x > 0) ? x : -x;
}

void* Cm_memset(void *ptr, int c, uintptr_t num)
{
    uint8_t *p = ptr;
    while(num--)
    {
        *p++ = (uint8_t)c;
    }
    return ptr;
}