#include "crc.h"

uint32_t RtlCrc32(uint32_t initial, uint32_t polynomial, const void *data, size_t size) 
{
    const uint8_t *d = data;
    uint32_t crc = initial;

    for(size_t i = 0; i < size; ++i)
    {
        crc = crc ^ d[i];
        for(int8_t k = 7; k >= 0; k--) 
        {
            crc = (crc >> 1) ^ (polynomial & (-(crc & 1)));
        }
    }
    return ~crc;
}