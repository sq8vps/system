#ifdef __GNUC__

#include <stdint.h>

__attribute__((weak, visibility("hidden")))
long double ldexp(long double value, int exponent)
{
#if __LDBL_MANT_DIG__ == 64
    union
    {
        long double ld;
        struct
        {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
            uint64_t fractionInteger;
            uint16_t exponentSign;
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
            uint16_t exponentSign;
            uint64_t fractionInteger;
#else
#error Unknown byte order
#endif
        } parts __attribute__((packed));
    } u;
    
    u.ld = value;
    
    uint32_t ex = u.parts.exponentSign & 0x7FFF;
    ex += exponent;
    u.parts.exponentSign &= ~0x7FFF;
    u.parts.exponentSign |= (ex & 0x7FFF); 

    return u.ld;
#elif __LDBL_MANT_DIG__ == 53
    union
    {
        long double ld;
        uint64_t u64;
    } u;
    
    u.ld = value;
    
    uint32_t ex = (u.u64 >> 52) & 0x7FF;
    ex += exponent;
    u.u64 &= ~((uint64_t)0x7FF << 52);
    u.u64 |= ((uint64_t)(ex & 0x7FF) << 52); 
    return u.ld;
#elif __LDBL_MANT_DIG__ == 24
    union
    {
        long double ld;
        uint32_t u32;
    } u;
    
    u.ld = value;
    uint32_t ex = (u.u32 >> 23) & 0xFF;
    ex += exponent;
    u.u32 &= ~((uint32_t)0xFF << 23);
    u.u32 |= ((uint32_t)(ex & 0xFF) << 23); 
    return u.ld;
#else
    #error Unknown long double size
#endif
}

__attribute__((weak, visibility("hidden")))
double nan(const char *tagp)
{
    return __builtin_nan(tagp);
}

__attribute__((weak, visibility("hidden")))
float nanf(const char *tagp)
{
    return __builtin_nanf(tagp);
}


#endif