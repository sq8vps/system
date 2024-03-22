#include "order.h"
#include <stdbool.h>

static bool CmIsBigEndian = false;

#pragma GCC push_options
#pragma GCC optimize ("O0")
void CmDetectEndianness(void)
{
    union
    {
        uint8_t b[2];
        uint16_t w;
    } t;

    t.w = 0xAABB;

    if(0xBB == t.b[0])
        CmIsBigEndian = false;
    else
        CmIsBigEndian = true;
}
#pragma GCC pop_options

uint16_t CmLeU16(uint16_t x)
{
    if(!CmIsBigEndian)
        return x;
    else
        return __builtin_bswap16(x);
}

int16_t CmLeS16(int16_t x)
{
    if(!CmIsBigEndian)
        return x;
    else
        return __builtin_bswap16(x);
}

uint32_t CmLeU32(uint32_t x)
{
    if(!CmIsBigEndian)
        return x;
    else
        return __builtin_bswap32(x);
}

int32_t CmLeS32(int32_t x)
{
    if(!CmIsBigEndian)
        return x;
    else
        return __builtin_bswap32(x);
}

uint64_t CmLeU64(uint64_t x)
{
    if(!CmIsBigEndian)
        return x;
    else
        return __builtin_bswap64(x);
}

int64_t CmLeS64(int64_t x)
{
    if(!CmIsBigEndian)
        return x;
    else
        return __builtin_bswap64(x);
}

uint16_t CmBeU16(uint16_t x)
{
    if(CmIsBigEndian)
        return x;
    else
        return __builtin_bswap16(x);
}

int16_t CmBeS16(int16_t x)
{
    if(CmIsBigEndian)
        return x;
    else
        return __builtin_bswap16(x);
}

uint32_t CmBeU32(uint32_t x)
{
    if(CmIsBigEndian)
        return x;
    else
        return __builtin_bswap32(x);
}

int32_t CmBeS32(int32_t x)
{
    if(CmIsBigEndian)
        return x;
    else
        return __builtin_bswap32(x);
}

uint64_t CmBeU64(uint64_t x)
{
    if(CmIsBigEndian)
        return x;
    else
        return __builtin_bswap64(x);
}

int64_t CmBeS64(int64_t x)
{
    if(CmIsBigEndian)
        return x;
    else
        return __builtin_bswap64(x);
}