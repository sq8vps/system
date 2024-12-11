#ifndef KERNEL_DEFINES_H_
#define KERNEL_DEFINES_H_

/**
 * @file defines.h
 * @brief Common kernel definitions and macros
 * 
 * Provides a set of common kernel definitions, typedefs and macros.
 * 
 * @defgroup defines Common kernel definitions
*/

#include <stddef.h>
#include <stdint.h>

/**
 * @ingroup defines
 * @{
*/

/**
 * @brief Export all following lines up to the \a END_EXPORT_API mark
 */
#define EXPORT_API

/**
 * @brief End to-be-exported block started with \a EXPORT_API
 */
#define END_EXPORT_API

EXPORT_API

/**
 * @brief Mark symbol (function/variable) as internal/hidden
*/
#define INTERNAL __attribute__ ((visibility("hidden")))

/**
 * @brief Stringify without expanding
 * @param ... Arguments to stringify
 * @return Stringified arguments
*/
#define _STRINGIFY(...) #__VA_ARGS__


/**
 * @brief Expand and stringify
 * @param ... Arguments to expand and stringify
 * @return Expanded and stringified arguments
*/
#define STRINGIFY(...) _STRINGIFY(__VA_ARGS__)

/**
 * @brief Mark branch as extremely likely for compiler optimization
 */
#define likely(x) __builtin_expect(!!(x), 1)

/**
 * @brief Mark branch as extremely unlikely for compiler optimization
 */
#define unlikely(x) __builtin_expect(!!(x), 0)

/**
 * @brief Divide two integers and round up
 */
#define CEIL_DIV(dividend, divisor) ((dividend) / (divisor) + (((dividend) % (divisor)) ? 1 : 0))

/**
 * @brief Kernel status codes
*/
typedef enum
{
    //OK response
    OK = 0,

    //common errors
    NULL_POINTER_GIVEN = 0x100,
    NOT_IMPLEMENTED,
    OUT_OF_RESOURCES,
    DEVICE_NOT_AVAILABLE,
    SYSTEM_INCOMPATIBLE,
    BAD_PARAMETER,
    OPERATION_NOT_ALLOWED,
    BAD_ALIGNMENT,
    NOT_COMPATIBLE,
    TIMEOUT,
    ILLEGAL_OPERATION,
    SYSCALL_CODE_UNKNOWN,
    INVALID_ARGUMENT,

    //interrupt module errors
    BAD_INTERRUPT_VECTOR, //bad interrupt vector number
    INTERRUPT_VECTOR_NOT_FREE,
    INTERRUPT_ALREADY_REGISTERED,
    INTERRUPT_NOT_REGISTERED,
    NO_FREE_INTERRUPT_VECTORS,

    //memory management module errors
    PAGE_NOT_PRESENT, //page not present in physical memory
    MEMORY_ALREADY_MAPPED, //page is already mapped to other virtual address
    MEMORY_ALREADY_UNMAPPED, //memory is already unmapped

    ELF_BAD_ARCHITECTURE,
    ELF_BAD_INSTRUCTION_SET,
    ELF_BAD_FORMAT,
    ELF_BAD_ENDIANESS,
    ELF_BROKEN,
    ELF_UNSUPPORTED_TYPE,
    ELF_UNDEFINED_SYMBOL,
    ELF_UNDEFINED_EXTERNAL_SYMBOL,
    DRIVER_INITIALIZATION_FAILED,
    DATABASE_BROKEN,
    DATABASE_ENTRY_NOT_FOUND,

    SCHEDULER_INITIALIZATION_FAILURE,
    KERNEL_THREAD_LIMIT_REACHED,

    ROOT_DEVICE_INIT_FAILURE,
    ILLEGAL_NAME,
    FILE_NOT_FOUND,
    FILE_ALREADY_EXISTS,
    NOT_A_DIRECTORY,
    FILE_COUNT_LIMIT_REACHED,
    BAD_FILE_TYPE,
    FILE_CLOSED,
    FILE_LOCKED,
    FILE_IRREMOVABLE,
    DIRECTORY_NOT_EMPTY,
    FILE_IN_USE,
    FILE_READ_ONLY,
    READ_INCOMPLETE,
    WRITE_INCOMPLETE,
    VFS_INITIALIZATION_FAILED,
    RP_NOT_CANCELLABLE,
    RP_PROCESSING_FAILED,
    RP_CODE_UNKNOWN,
    VOLUME_NOT_REGISTERED,
    VOLUME_ALREADY_EXISTS,
    VOLUME_ALREADY_MOUNTED,
    VOLUME_TOO_SMALL,
    UNKNOWN_FILE_SYSTEM,
    FILE_BROKEN,
    FILE_BAD_MODE,
    FILE_TOO_SMALL,
    IOCTL_UNKNOWN,

    UNKOWN_OBJECT_TYPE,

    UNKNOWN_ERROR = 0xFFFFFFFF,

} STATUS;


/**
 * @brief General privilege level enum
*/
typedef enum PrivilegeLevel
{
    PL_KERNEL,
    PL_USER
} PrivilegeLevel;


/**
 * @brief A common timestamp type
*/
typedef uint64_t time_t;



/**
 * @brief Attribute for never-returning functions
*/
#define NORETURN __attribute__((noreturn))


/**
 * @brief Attribute for packed structures
*/
#define PACKED __attribute__ ((packed))


/**
 * @brief Variable alignment macro
 * @param n Alignment value in bytes
*/
#define ALIGN(n) __attribute__ ((aligned(n)))


/**
 * @brief Align value up
 * @param val Value to be aligned
 * @param mask Alignment bitmask
*/
#define ALIGN_UP_MASK(val, mask) (((val) + (mask)) & ~(mask))


/**
 * @brief Align value up
 * @param val Value to be aligned
 * @param align Alignment value
*/
#define ALIGN_UP(val, align) ALIGN_UP_MASK(val, (typeof(val))(align) - 1)


/**
 * @brief Align value down
 * @param val Value to be aligned
 * @param align Alignment value
*/
#define ALIGN_DOWN(val, align) ((val) & ~((typeof(val))(align) - 1))


/**
 * @brief Macro for inline assembly
*/
#define ASM asm volatile

/**
 * @brief Memory barrier
 */
#define barrier() ASM("" ::: "memory")

/**
 * @brief Check if character \a x is alphanumeric
 * @param x Character to check
 * @return 1 if is alphanumeric, 0 otherwise
*/
#define IS_ALPHANUMERIC(x) ((((x) >= '0') && ((x) <= '9')) || (((x) >= 'A') && ((x) <= 'Z')) || (((x) >= 'a') && ((x) <= 'z')))


/**
 * @brief Convert microseconds to nanoseconds (standard kernel time resolution)
 * @param us Value in microseconds
 * @return Value in nanoseconds
*/
#define US_TO_NS(us) (((uint64_t)1000) * (us))


/**
 * @brief Convert milliseconds to nanoseconds (standard kernel time resolution)
 * @param us Value in milliseconds
 * @return Value in nanoseconds
*/
#define MS_TO_NS(ms) (((uint64_t)1000000) * (ms))


/**
 * @brief Unique Identifier structure
*/
union UID
{
    struct
    {
        struct
        {
            uint32_t high;
            uint16_t low;
        } time PACKED;
        uint16_t reserved;
        uint8_t family;
        uint8_t node[7];
    } PACKED;
    uint8_t raw[16];
} PACKED;

END_EXPORT_API

#include "hal/arch.h"

/**
 * @}
*/

#endif