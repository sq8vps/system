#ifndef UNICODE_H_
#define UNICODE_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define UNICODE_ZWJ (uint32_t)0x200D /**< Zero-width joiner */

/**
 * @brief Get UTF-8 byte count
 * @param c First UTF-8/ASCII character
 * @return Number of bytes in UTF-8 character
 * @return Always 1 if ASCII character
 */
static inline size_t Utf8ByteCount(char c)
{
    if(((c) & 0xE0) == 0xC0)
        return 2;
    else if(((c) & 0xF0) == 0xE0)
        return 3;
    else if(((c) & 0xF8) == 0xF0)
        return 4;
    else
        return 1;
}

static inline bool UnicodeIsCombining(uint32_t code) 
{
    return (code >= 0x0300 && code <= 0x036F)
        || (code >= 0x1AB0 && code <= 0x1AFF)
        || (code >= 0x1DC0 && code <= 0x1DFF)
        || (code >= 0x20D0 && code <= 0x20FF)
        || (code >= 0xFE20 && code <= 0xFE2F);
}

static inline bool UnicodeIsVariationSelector(uint32_t code) 
{
    return (code >= 0xFE00 && code <= 0xFE0F)
        || (code >= 0xE0100 && code <= 0xE01EF);
}

static inline bool UnicodeIsExtending(uint32_t code) 
{
    return UnicodeIsCombining(code) || UnicodeIsVariationSelector(code);
}

static bool UnicodeIsGraphemeClusterStart(uint32_t previous, uint32_t current)
{
    if(UnicodeIsExtending(current))
        return false;

    if(UNICODE_ZWJ == previous)
        return false;

    return true;
}

static size_t UnicodeGraphemeClusterLength(const uint32_t *code, size_t size) 
{
    if(0 == size)
        return 0;

    size_t i = 1;
    while (i < size) 
    {
        if (UnicodeIsExtending(code[i])) 
        {
            i++;
        } 
        else if (UNICODE_ZWJ == code[i - 1]) 
        {
            i++;
        } 
        else 
        {
            break;
        }
    }

    return i;
}

#endif