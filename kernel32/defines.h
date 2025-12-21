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
#include "status.h"

/**
 * @ingroup defines
 * @{
*/

/**
 * @brief Export all following lines up to the #END_EXPORT_SYSCALL mark as user-mode syscall interface
 */
#define EXPORT_SYSCALL

/**
 * @brief End to-be-exported block started with #EXPORT_SYSCALL
 */
#define END_EXPORT_SYSCALL

EXPORT_API

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
 * @brief Mark function parameter as unused
 */
#define UNUSED(x) (void)(x)

/**
 * @brief Mark symbol (function/variable) as internal/hidden
*/
#define INTERNAL __attribute__ ((visibility("hidden")))

/**
 * @brief Mark symbol as weak/overridable
 */
#define WEAK __attribute__ ((weak))

/**
 * @brief Mark function as frequently called for compiler optimization
 */
#define HOT __attribute__ ((hot))

/**
 * @brief Mark function as deprecated
 */
#define DEPRECATED __attribute__ ((deprecated))

/**
 * @brief Mark case as fallthrough
 */
#define FALLTHROUGH __attribute__ ((fallthrough))

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
 * @brief No wait
*/
#define NO_WAIT 0

/**
 * @brief Wait indefinitely
*/
#define NO_TIMEOUT UINT64_MAX

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