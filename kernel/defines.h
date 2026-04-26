/**
 * @file defines.h
 * @brief Global kernel definitions and macros
 * @ingroup defines
 */

#ifndef KERNEL_DEFINES_H_
#define KERNEL_DEFINES_H_

#include "platform/platform.h"

NABLA_API
#include <stddef.h>
#include <stdint.h>
END_NABLA_API

/**
 * @addtogroup defines Global kernel definitions and macros
 * @{
*/

DRIVER_API

NABLA_API

/**
 * @brief Kernel status codes
 * @note Function return these values directly - as positive integers
*/
typedef enum STATUS
{
    OK = 0, /**< Everything is OK */

    BAD_PARAMETER = 1, /**< Operation parameter is incorrect */
    NOT_IMPLEMENTED = 2, /**< Operation is either explicitly not implemented or not supported regardless of other parameters */
    NOT_SUPPORTED = 3, /**< Operation is not supported on given resource, probably because of other parameters */
    OUT_OF_RESOURCES = 4, /**< Operation size is bigger than available resources, e.g., out of memory, buffer too small, list full, out of IDs. */
    DEVICE_NOT_AVAILABLE = 5, /**< Device does not exist or is not available for given operation, e.g., because the owner changed */
    BAD_ALIGNMENT = 6, /**< Bad memory alignment or offset */
    TIMEOUT = 7, /**< Operation timed out */
    BUSY = 8, /**< Resource is busy, e.g, there is an ongoing operation, resource cannot be shared, etc. */
    ALREADY_EXISTS = 9, /**< Resource already exists */
    PAGE_NOT_PRESENT = 10, /**< Page is not present in physical memory */
    CORRUPTED = 11, /**< Resource exists, but is corrupted */
    UNDEFINED_SYMBOL = 12, /**< Symbol is undefined */
    NOT_FOUND = 13, /**< Resource not found */
    BAD_TYPE = 14, /**< Bad object/resource/file/... type */
    FILE_CLOSED = 15, /**< File is not open */
    RESOURCE_BOUND = 16, /**< Resource is bound (to a peer, parent, or child) and can't be manipulated freely */
    READ_ONLY = 17, /**< Resource is read-only */
    OPERATION_INCOMPLETE = 18, /**< Operation finished but with incomplete data. This might or might not be an error */
    RESOURCE_PERSISTENT = 19, /**< Resource is persistent and cannot be removed */

    UNKNOWN_ERROR = 0x1000, /**< Unknown or undefined error */
} STATUS;

/**
 * @brief Stringify without expanding
 * @param ... Arguments to stringify
 * @return Stringified arguments
*/
#define STRINGIFY_NO_EXPAND(...) #__VA_ARGS__

/**
 * @brief Expand and stringify
 * @param ... Arguments to expand and stringify
 * @return Expanded and stringified arguments
*/
#define STRINGIFY(...) STRINGIFY_NO_EXPAND(__VA_ARGS__)

/**
 * @brief Maximum number of arguments a function can accept
 */
#define MAX_ARG_COUNT 10

/**
 * @brief Internal function for obtaining number of macro arguments
 * @attention Use \ref ARG_COUNT()
 * @note Credits to Laurent Deniau
 */
#define __ARG_COUNT(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, N, ...) N

/**
 * @brief Internal function for obtaining number of macro arguments
 * @attention Use \ref ARG_COUNT()
 * @note Credits to Laurent Deniau
 */
#define _ARG_COUNT(...) __ARG_COUNT(__VA_ARGS__)

/**
 * @brief Get number of arguments in a variadic macro
 * @param ... Arguments to count
 * @return Number of arguments given
 * @note Credits to Laurent Deniau
 */
#define ARG_COUNT(...) _ARG_COUNT(__VA_OPT__(__VA_ARGS__,) 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)

/**
 * @brief Divide two integers and round up
 */
#define CEIL_DIV(dividend, divisor) ((dividend) / (divisor) + (((dividend) % (divisor)) ? 1 : 0))


/**
 * @brief Mark function parameter as unused
 */
#define UNUSED(x) (void)(x)

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

END_NABLA_API

/**
 * @brief A common timestamp type
*/
typedef uint64_t time_t;

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

/**
 * @brief General privilege level enum
*/
typedef enum
{
    PL_KERNEL = 0, /**< Kernel mode */
    PL_USER = 1, /**< User mode */
} PrivilegeLevel;

END_DRIVER_API

#ifdef SMP

/**
 * @brief Do whathever's inside the argument list when SMP is enabled
 * @param ... Things to do when SMP is enabled
 */
#define SMP_ONLY(...) __VA_ARGS__
#else

/**
 * @brief Do whathever's inside the argument list when SMP is enabled
 * @param ... Things to do when SMP is enabled
 */
#define SMP_ONLY(...)
#endif

#include "hal/arch.h"

/**
 * @}
*/

#endif