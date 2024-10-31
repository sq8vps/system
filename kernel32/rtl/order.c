#include "order.h"
#include <stdbool.h>

static bool RtlIsBigEndian = false;

#pragma GCC push_options
#pragma GCC optimize ("O0")
void RtlDetectEndianness(void)
{
    union
    {
        uint8_t b[2];
        uint16_t w;
    } t;

    t.w = 0xAABB;

    if(0xBB == t.b[0])
        RtlIsBigEndian = false;
    else
        RtlIsBigEndian = true;
}
#pragma GCC pop_options

uint16_t RtlLeU16(uint16_t x)
{
    if(!RtlIsBigEndian)
        return x;
    else
        return __builtin_bswap16(x);
}

int16_t RtlLeS16(int16_t x)
{
    if(!RtlIsBigEndian)
        return x;
    else
        return __builtin_bswap16(x);
}

uint32_t RtlLeU32(uint32_t x)
{
    if(!RtlIsBigEndian)
        return x;
    else
        return __builtin_bswap32(x);
}

int32_t RtlLeS32(int32_t x)
{
    if(!RtlIsBigEndian)
        return x;
    else
        return __builtin_bswap32(x);
}

uint64_t RtlLeU64(uint64_t x)
{
    if(!RtlIsBigEndian)
        return x;
    else
        return __builtin_bswap64(x);
}

int64_t RtlLeS64(int64_t x)
{
    if(!RtlIsBigEndian)
        return x;
    else
        return __builtin_bswap64(x);
}

uint16_t RtlBeU16(uint16_t x)
{
    if(RtlIsBigEndian)
        return x;
    else
        return __builtin_bswap16(x);
}

int16_t RtlBeS16(int16_t x)
{
    if(RtlIsBigEndian)
        return x;
    else
        return __builtin_bswap16(x);
}

uint32_t RtlBeU32(uint32_t x)
{
    if(RtlIsBigEndian)
        return x;
    else
        return __builtin_bswap32(x);
}

int32_t RtlBeS32(int32_t x)
{
    if(RtlIsBigEndian)
        return x;
    else
        return __builtin_bswap32(x);
}

uint64_t RtlBeU64(uint64_t x)
{
    if(RtlIsBigEndian)
        return x;
    else
        return __builtin_bswap64(x);
}

int64_t RtlBeS64(int64_t x)
{
    if(RtlIsBigEndian)
        return x;
    else
        return __builtin_bswap64(x);
}