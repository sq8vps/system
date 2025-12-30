//This header file is generated automatically
#ifndef EXPORTED___API__DEFINES_H_
#define EXPORTED___API__DEFINES_H_

#ifdef __cplusplus
extern "C" 
{
#endif

#include <stddef.h>
#include <stdint.h>
#include "status.h"
#include "platform/platform.h"

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

#include "hal/arch.h"

#ifdef __cplusplus
}
#endif

#endif