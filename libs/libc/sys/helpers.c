#include "defines.h"
#include "sys/errno.h"
#include <stdint.h>

int NablaStatusToErrno(STATUS status)
{
    switch(status)
    {
        case OK:
            return 0;
        case BAD_PARAMETER:
            return EINVAL;
        case NOT_IMPLEMENTED:
            return ENOSYS;
        case NOT_SUPPORTED:
            return EINVAL;
        case OUT_OF_RESOURCES:
            return ENOMEM;
        case DEVICE_NOT_AVAILABLE:
            return ENODEV;
        case BAD_ALIGNMENT:
            return EINVAL;
        case TIMEOUT:
            return ETIMEDOUT;
        case BUSY:
            return EBUSY;
        case ALREADY_EXISTS:
            return EEXIST;
        case CORRUPTED:
            return EIO;
        case NOT_FOUND:
            return ENOENT;
        case BAD_TYPE:
            return EINVAL;
        case FILE_CLOSED:
            return EBADF;
        case RESOURCE_BOUND_OR_LOCKED:
            return EBUSY;
        case READ_ONLY:
            return EROFS;
        case OPERATION_INCOMPLETE:
            return EIO;
        case RESOURCE_PERSISTENT:
            return EPERM;
        case UNKNOWN_ERROR:
            return EIO;
        //don't really expect these in user mode
        case PAGE_NOT_PRESENT:
        case UNDEFINED_SYMBOL:
            return EIO;
    }

    return EIO;
}

int __nabla_init_libc(int argc, char **argv, char **envp, void *progData)
{
    return 0;
}

#ifdef __GNUC__

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