#include "stdlib.h"
#include "ctype.h"
#include <limits.h>
#include "errno.h"

int strtoi(const char *restrict nptr, char **restrict endptr, int base)
{
    const char *s = nptr;
    int c = 0;
    bool negative = false;
    bool overflow = false;
    bool empty = true;
    int result = 0;

    do
    {
        c = *s++;
    } 
    while(isspace(c));

    if('-' == c)
    {
        negative = true;
        c = *s++;
    }
    else if('+' == c)
    {
        c = *s++;
    }

    if('0' == c)
    {
        if(('x' == *s) || ('X' == *s))
        {
            s += 2;
            if(0 == base)
                base = 16;
        }
        else if(0 == base)
        {
            ++s;
            base = 8;
        }
    }
    else if(0 == base)
        base = 10;

    int maxBefore = (negative ? -(unsigned int)INT_MIN : INT_MAX) / base;
    int maxRemainder = (negative ? -(unsigned int)INT_MIN : INT_MAX) % base;

    while(1)
    {
        if(isdigit(c))
            c -= '0';
        else if(isalpha(c))
            c -= (isupper(c) ? 'A' : 'a') - 10;
        else
            break;
        
        if(c >= base)
            break;

        empty = false;

        if(overflow || (result > maxBefore) || ((result == maxBefore) && (c > maxRemainder)))
        {
            overflow = true;
        }
        else
        {
            result *= base;
            result += c;
        }
        
        c = *s++;
    }

    if(negative)
        result = -result;
    
    if(nullptr != endptr)
        *endptr = (char*)(empty ? nptr : (s - 1));
    
    if(overflow)
    {
        errno = -ERANGE;
        result = negative ? INT_MIN : INT_MAX;
    }

    return result;
}

long int strtol(const char *restrict nptr, char **restrict endptr, int base)
{
    const char *s = nptr;
    int c = 0;
    bool negative = false;
    bool overflow = false;
    bool empty = true;
    long result = 0;

    do
    {
        c = *s++;
    } 
    while(isspace(c));

    if('-' == c)
    {
        negative = true;
        c = *s++;
    }
    else if('+' == c)
    {
        c = *s++;
    }

    if('0' == c)
    {
        if(('x' == *s) || ('X' == *s))
        {
            s += 2;
            if(0 == base)
                base = 16;
        }
        else if(0 == base)
        {
            ++s;
            base = 8;
        }
    }
    else if(0 == base)
        base = 10;

    long maxBefore = (negative ? -(unsigned long)LONG_MIN : LONG_MAX) / base;
    long maxRemainder = (negative ? -(unsigned long)LONG_MIN : LONG_MAX) % base;

    while(1)
    {
        if(isdigit(c))
            c -= '0';
        else if(isalpha(c))
            c -= (isupper(c) ? 'A' : 'a') - 10;
        else
            break;
        
        if(c >= base)
            break;

        empty = false;

        if(overflow || (result > maxBefore) || ((result == maxBefore) && (c > maxRemainder)))
        {
            overflow = true;
        }
        else
        {
            result *= base;
            result += c;
        }
        
        c = *s++;
    }

    if(negative)
        result = -result;
    
    if(nullptr != endptr)
        *endptr = (char*)(empty ? nptr : (s - 1));
    
    if(overflow)
    {
        errno = -ERANGE;
        result = negative ? LONG_MIN : LONG_MAX;
    }

    return result;
}

long long int strtoll(const char *restrict nptr, char **restrict endptr, int base)
{
    const char *s = nptr;
    int c = 0;
    bool negative = false;
    bool overflow = false;
    bool empty = true;
    long long result = 0;

    do
    {
        c = *s++;
    } 
    while(isspace(c));

    if('-' == c)
    {
        negative = true;
        c = *s++;
    }
    else if('+' == c)
    {
        c = *s++;
    }

    if('0' == c)
    {
        if(('x' == *s) || ('X' == *s))
        {
            s += 2;
            if(0 == base)
                base = 16;
        }
        else if(0 == base)
        {
            ++s;
            base = 8;
        }
    }
    else if(0 == base)
        base = 10;

    long long maxBefore = (negative ? -(unsigned long long)LLONG_MIN : LLONG_MAX) / base;
    long long maxRemainder = (negative ? -(unsigned long long)LLONG_MIN : LLONG_MAX) % base;

    while(1)
    {
        if(isdigit(c))
            c -= '0';
        else if(isalpha(c))
            c -= (isupper(c) ? 'A' : 'a') - 10;
        else
            break;
        
        if(c >= base)
            break;

        empty = false;

        if(overflow || (result > maxBefore) || ((result == maxBefore) && (c > maxRemainder)))
        {
            overflow = true;
        }
        else
        {
            result *= base;
            result += c;
        }
        
        c = *s++;
    }

    if(negative)
        result = -result;
    
    if(nullptr != endptr)
        *endptr = (char*)(empty ? nptr : (s - 1));
    
    if(overflow)
    {
        errno = -ERANGE;
        result = negative ? LLONG_MIN : LLONG_MAX;
    }

    return result;
}

unsigned int strtoui(const char *restrict nptr, char **restrict endptr, int base)
{
    const char *s = nptr;
    int c = 0;
    bool negative = false;
    bool overflow = false;
    bool empty = true;
    unsigned int result = 0;

    do
    {
        c = *s++;
    } 
    while(isspace(c));

    if('-' == c)
    {
        negative = true;
        c = *s++;
    }
    else if('+' == c)
    {
        c = *s++;
    }

    if('0' == c)
    {
        if(('x' == *s) || ('X' == *s))
        {
            s += 2;
            if(0 == base)
                base = 16;
        }
        else if(0 == base)
        {
            ++s;
            base = 8;
        }
    }
    else if(0 == base)
        base = 10;

    unsigned int maxBefore = UINT_MAX / base;
    unsigned int maxRemainder = UINT_MAX % base;

    while(1)
    {
        if(isdigit(c))
            c -= '0';
        else if(isalpha(c))
            c -= (isupper(c) ? 'A' : 'a') - 10;
        else
            break;
        
        if(c >= base)
            break;

        empty = false;

        if(overflow || (result > maxBefore) || ((result == maxBefore) && ((unsigned int)c > maxRemainder)))
        {
            overflow = true;
        }
        else
        {
            result *= base;
            result += c;
        }
        
        c = *s++;
    }

    if(negative)
        result = -result;
    
    if(nullptr != endptr)
        *endptr = (char*)(empty ? nptr : (s - 1));
    
    if(overflow)
    {
        errno = -ERANGE;
        result = UINT_MAX;
    }

    return result;
}

unsigned long int strtoul(const char *restrict nptr, char **restrict endptr, int base)
{
    const char *s = nptr;
    int c = 0;
    bool negative = false;
    bool overflow = false;
    bool empty = true;
    unsigned long result = 0;

    do
    {
        c = *s++;
    } 
    while(isspace(c));

    if('-' == c)
    {
        negative = true;
        c = *s++;
    }
    else if('+' == c)
    {
        c = *s++;
    }

    if('0' == c)
    {
        if(('x' == *s) || ('X' == *s))
        {
            s += 2;
            if(0 == base)
                base = 16;
        }
        else if(0 == base)
        {
            ++s;
            base = 8;
        }
    }
    else if(0 == base)
        base = 10;

    unsigned long maxBefore = ULONG_MAX / base;
    unsigned long maxRemainder = ULONG_MAX % base;

    while(1)
    {
        if(isdigit(c))
            c -= '0';
        else if(isalpha(c))
            c -= (isupper(c) ? 'A' : 'a') - 10;
        else
            break;
        
        if(c >= base)
            break;

        empty = false;

        if(overflow || (result > maxBefore) || ((result == maxBefore) && ((unsigned int)c > maxRemainder)))
        {
            overflow = true;
        }
        else
        {
            result *= base;
            result += c;
        }
        
        c = *s++;
    }

    if(negative)
        result = -result;
    
    if(nullptr != endptr)
        *endptr = (char*)(empty ? nptr : (s - 1));
    
    if(overflow)
    {
        errno = -ERANGE;
        result = ULONG_MAX;
    }

    return result;
}

unsigned long long int strtoull(const char *restrict nptr, char **restrict endptr, int base)
{
    const char *s = nptr;
    int c = 0;
    bool negative = false;
    bool overflow = false;
    bool empty = true;
    unsigned long long result = 0;

    do
    {
        c = *s++;
    } 
    while(isspace(c));

    if('-' == c)
    {
        negative = true;
        c = *s++;
    }
    else if('+' == c)
    {
        c = *s++;
    }

    if('0' == c)
    {
        if(('x' == *s) || ('X' == *s))
        {
            s += 2;
            if(0 == base)
                base = 16;
        }
        else if(0 == base)
        {
            ++s;
            base = 8;
        }
    }
    else if(0 == base)
        base = 10;

    unsigned long long maxBefore = ULLONG_MAX / base;
    unsigned long long maxRemainder =  ULLONG_MAX % base;

    while(1)
    {
        if(isdigit(c))
            c -= '0';
        else if(isalpha(c))
            c -= (isupper(c) ? 'A' : 'a') - 10;
        else
            break;
        
        if(c >= base)
            break;

        empty = false;

        if(overflow || (result > maxBefore) || ((result == maxBefore) && ((unsigned int)c > maxRemainder)))
        {
            overflow = true;
        }
        else
        {
            result *= base;
            result += c;
        }
        
        c = *s++;
    }

    if(negative)
        result = -result;
    
    if(nullptr != endptr)
        *endptr = (char*)(empty ? nptr : (s - 1));
    
    if(overflow)
    {
        errno = -ERANGE;
        result = ULLONG_MAX;
    }

    return result;
}

int atoi(const char *nptr)
{
    const char *s = nptr;
    int c = 0;
    bool negative = false;
    int result = 0;

    do
    {
        c = *s++;
    } 
    while(isspace(c));

    if('-' == c)
    {
        negative = true;
        c = *s++;
    }
    else if('+' == c)
    {
        c = *s++;
    }

    while(1)
    {
        if(isdigit(c))
            c -= '0';
        else
            break;

        result *= 10;
        result += c;
        
        c = *s++;
    }

    if(negative)
        result = -result;

    return result;    
}

long atol(const char *nptr)
{
    const char *s = nptr;
    int c = 0;
    bool negative = false;
    long result = 0;

    do
    {
        c = *s++;
    } 
    while(isspace(c));

    if('-' == c)
    {
        negative = true;
        c = *s++;
    }
    else if('+' == c)
    {
        c = *s++;
    }

    while(1)
    {
        if(isdigit(c))
            c -= '0';
        else
            break;

        result *= 10;
        result += c;
        
        c = *s++;
    }

    if(negative)
        result = -result;

    return result;    
}

long long atoll(const char *nptr)
{
    const char *s = nptr;
    int c = 0;
    bool negative = false;
    long long result = 0;

    do
    {
        c = *s++;
    } 
    while(isspace(c));

    if('-' == c)
    {
        negative = true;
        c = *s++;
    }
    else if('+' == c)
    {
        c = *s++;
    }

    while(1)
    {
        if(isdigit(c))
            c -= '0';
        else
            break;

        result *= 10;
        result += c;
        
        c = *s++;
    }

    if(negative)
        result = -result;

    return result;    
}