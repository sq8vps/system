#include "common.h"
#include "defines.h"
#include <stdarg.h>

uint32_t CmStrlen(const char *str)
{
    char *s = (char*)str;
    while(*s)
        s++;
    
    return (uintptr_t)s - (uintptr_t)str;
}

char* CmStrcpy(char *strTo, const char *strFrom)
{
    char *initial = strTo;
    while(0 != (*strTo++ = *strFrom++))
        ;
    return initial;
}

char* CmStrncpy(char *strTo, const char *strFrom, uintptr_t n)
{
    char *initial = strTo;
    while(0 != (*strTo++ = *strFrom++))
    {
        n--;
        if(n == 0)
        {
            *strTo = 0;
            break;
        }
    }
    return initial;
}

int CmStrcmp(const char *s1, const char *s2)
{
    while(*s1 && (*s1 == *s2))
    {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

int CmStrncmp(const char *s1, const char *s2, int n)
{
    while (n && *s1 && (*s1 == *s2))
    {
        s1++;
        s2++;
        n--;
    }
    if(0 == n)
        return 0;
    

    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

void* CmMemcpy(void *to, const void *from, uintptr_t n)
{
    uint8_t *cto = (uint8_t*)to;
    const uint8_t *cfrom =  (const uint8_t*)from;
    while(n)
    {
        *(cto++) = *(cfrom++);
        --n;
    }
    return to;
}

uint32_t CmAbs(int32_t x)
{
    return (x > 0) ? x : -x;
}

void* CmMemset(void *ptr, int c, uintptr_t num)
{
    uint8_t *p = ptr;
    while(num)
    {
        *p++ = (uint8_t)c;
        num--;
    }
    return ptr;
}