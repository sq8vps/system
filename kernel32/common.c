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
    while(n && *s1 && (*s1 == *s2))
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

uint64_t CmAbs(int64_t x)
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

int CmMemcmp(const void *s1, const void *s2, uintptr_t n)
{
    register const unsigned char *str1 = (const unsigned char*)s1;
    register const unsigned char *str2 = (const unsigned char*)s2;

    while (n-- > 0)
    {
        if (*str1++ != *str2++)
        return str1[-1] < str2[-1] ? -1 : 1;
    }
    return 0;
}

int CmIsprint(int c)
{
	return ((unsigned int)c - 0x20) < 0x5f;
}

char *CmStrcat(char *dst, const char *src)
{
    char* ptr = dst + CmStrlen(dst);
 
    while (*src != '\0') 
    {
        *ptr++ = *src++;
    }
 
    *ptr = '\0';

    return dst;
}

char CmIsxdigit(int c)
{
    if ((c >= (int)'0' && c <= (int)'9') ||
         (c >= (int)'a' && c <= (int)'f') ||
         (c >= (int)'A' && c <= (int)'F'))
        return 1;

    return 0;
}

char CmIsdigit(int c)
{
    if ((c >= (int)'0' && c <= (int)'9'))
        return 1;

    return 0;
}

int CmIsspace(int c)
{
	if(c == (int)'\t' || c == (int)'\n' || c == (int)'\v' || c == (int)'\f' || c == (int)'\r' || c == (int)' ')
        return 1;
    
    return 0;
}

int CmTolower(int c)
{
    if((c >= (int)'A') && (c <= (int)'Z'))
        return c + 32;
    
    return c;
}

int CmToupper(int c)
{
    if((c >= (int)'a') && (c <= (int)'z'))
        return c - 32;
    
    return c;
}

char *CmGetFileName(char *path)
{
    uint32_t size = CmStrlen(path);
    if(0 == size)
        return path;

    if('/' == path[size - 1])
    {
        path[size - 1] = '\0';
        size--;
    }
    while(size > 0)
    {
        if('/' == path[size - 1])
            break;
        size--;
    }
    return &path[size];
}

uint64_t CmPow10(uint16_t x)
{
    uint64_t y = 1;
    while(x--)
    {
        y *= 10;
    }
    return y;
}