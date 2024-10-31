#include "utils.h"
#include <stdint.h>
#include "rtl/ctype.h"

int FatUcs2ToUtf8(char *utf8, const char *ucs2, size_t limit)
{
    size_t i = 0;
    while((0 != ucs2[0]) || (0 != ucs2[1]))
    {
        uint16_t ucs = (uint16_t)ucs2[0] | ((uint16_t)ucs2[1] << 8);
        if(ucs < 0x80)
        {
            if(i == limit)
                return -1;
            utf8[i++] = ucs;
        }
        else if(ucs < 0x800)
        {
            if((limit - i) < 1)
                return -1;
            
            utf8[i++] = ((ucs >> 6) & 0x1F) | 0xC0;
            utf8[i++] = (ucs & 0x3F) | 0x80;
        }
        else
        {
            if((limit - i) < 2)
                return -1;
            
            utf8[i++] = ((ucs >> 12) & 0xF) | 0xE0;
            utf8[i++] = ((ucs >> 6) & 0x3F) | 0x80;
            utf8[i++] = (ucs & 0x3F) | 0x80;
        }
        ucs2 += 2;
    }
    utf8[i] = '\0';
    return 0;
}

int FatCompareUcs2AndUtf8(const char *ucs2, const char *utf8)
{
    while(((ucs2[0] != 0) || (ucs2[1] != 0)) && (utf8[0] != 0))
    {
        uint16_t ucs = (uint16_t)ucs2[0] | ((uint16_t)ucs2[1] << 8);
        if(ucs < 0x80)
        {
            if(ucs != *utf8)
                break;
            
            utf8++;
        }
        else if(ucs < 0x800)
        {
            if(0 == utf8[1])
                break;
            if((((ucs & 0x3F) | 0x80) != utf8[1])
                || ((((ucs >> 6) & 0x1F) | 0xC0) != utf8[0]))
                break;
            utf8 += 2;
        }
        else
        {
            if((0 == utf8[1]) || (0 == utf8[2]))
                break;
            if((((ucs & 0x3F) | 0x80) != utf8[2])
                || ((((ucs >> 6) & 0x3F) | 0x80) != utf8[1])
                || ((((ucs >> 12) & 0xF) | 0xE0) != utf8[0]))
                break;
            utf8 += 3;
        }
        ucs2 += 2;
    }
    return (*((uint16_t*)ucs2) - *utf8);
}

int FatFileNameToDosName(const char *name, char *dosName)
{
    for(uint8_t i = 0; i < 11; i++)
        dosName[i] = ' ';
    dosName[11] = '\0';

    uint8_t i = 0, k = 0;
    while('.' != name[i])
    {
        if('\0' == name[i])
        {
            if(0 != i)
                return 0;
            else
                return -1; //empty string is not allowed
        }

        if(k == 8)
            return -1;

        dosName[k] = RtlToupper(name[i]);
        
        k++;
        i++;
    }

    k = 8;
    i++; //skip dot

    while('\0' != name[i])
    {
        if(k == 11)
            return -1; 

        dosName[k] = RtlToupper(name[i]);
        
        k++;
        i++;
    }
    return 0;
}

int FatDosNameToFileName(char *name, const char *dosName)
{
    uint8_t k = 0;
    for(int8_t i = 7; i >= 0; i--)
    {
        if(' ' != dosName[i])
        {
            name[i] = RtlTolower(dosName[i]);
            k++;
        }
    }

    if(' ' != dosName[8])
    {
        name[k++] = '.';
        for(uint8_t i = 8; i < 11; i++)
        {
            if(' ' != dosName[i])
                name[k++] = RtlTolower(dosName[i]);
        }
    }
    name[k] = '\0';

    return (k > 0) ? 0 : -1;
}