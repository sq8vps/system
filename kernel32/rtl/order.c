#include "order.h"
#include <stdbool.h>

static bool RtlIsBigEndian = false;

void RtlDetectEndianness(void)
{
    uint16_t a = 0xAABB;
    volatile uint8_t *b = (volatile uint8_t*)&a;
    if(0xBB == *b)
        RtlIsBigEndian = false;
    else
        RtlIsBigEndian = true;
}

uint16_t RtlLeU16(uint16_t x)
{
    if(!RtlIsBigEndian)
        return x;
    else
        return BSWAP(x);
}

int16_t RtlLeS16(int16_t x)
{
    if(!RtlIsBigEndian)
        return x;
    else
        return BSWAP(x);
}

uint32_t RtlLeU32(uint32_t x)
{
    if(!RtlIsBigEndian)
        return x;
    else
        return BSWAP(x);
}

int32_t RtlLeS32(int32_t x)
{
    if(!RtlIsBigEndian)
        return x;
    else
        return BSWAP(x);
}

uint64_t RtlLeU64(uint64_t x)
{
    if(!RtlIsBigEndian)
        return x;
    else
        return BSWAP(x);
}

int64_t RtlLeS64(int64_t x)
{
    if(!RtlIsBigEndian)
        return x;
    else
        return BSWAP(x);
}

uint16_t RtlBeU16(uint16_t x)
{
    if(RtlIsBigEndian)
        return x;
    else
        return BSWAP(x);
}

int16_t RtlBeS16(int16_t x)
{
    if(RtlIsBigEndian)
        return x;
    else
        return BSWAP(x);
}

uint32_t RtlBeU32(uint32_t x)
{
    if(RtlIsBigEndian)
        return x;
    else
        return BSWAP(x);
}

int32_t RtlBeS32(int32_t x)
{
    if(RtlIsBigEndian)
        return x;
    else
        return BSWAP(x);
}

uint64_t RtlBeU64(uint64_t x)
{
    if(RtlIsBigEndian)
        return x;
    else
        return BSWAP(x);
}

int64_t RtlBeS64(int64_t x)
{
    if(RtlIsBigEndian)
        return x;
    else
        return BSWAP(x);
}