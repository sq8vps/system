/**
 * @file stdio.h
 * @brief Kernel \c stdio.h implementation
 * @ingroup rtl_stdio
 */

#ifndef KERNEL_STDIO_H_
#define KERNEL_STDIO_H_

#include "defines.h"
#include <stdarg.h>

/**
 * @addtogroup rtl_stdio Kernel \c stdio.h implementation
 * @ingroup rtl
 * @note To disable aliasing RTL-specific names with C-standard names, define \c DISABLE_KERNEL_STDLIB before including this header.
 * @warning Note that kernel does not provide any standard output/input streams.
 * @warning This implementation does not adhere to any C standard.
 * @{
 */

EXPORT_API

/**
 * @brief Compose a formatted string
 * @param *format Format string
 * @param ... Additional arguments
 * @return Count of characters written
*/
PRINTF_LIKE(2, 3)
int RtlSprint(char * restrict s, const char * restrict format, ...);

/**
 * @brief Compose a formatted string
 * @param *format Format string
 * @param va_list Argument list
 * @return Count of characters written
*/
int RtlSprintV(char * restrict s, const char * restrict format, va_list args);


/**
 * @brief Compose a formatted string with length limit
 * @param *format Format string
 * @param n Length limit including terminator character
 * @param ... Additional arguments
 * @return Count of characters written
*/
PRINTF_LIKE(3, 4)
int RtlSprintN(char * restrict s, size_t n, const char * restrict format, ...);

/**
 * @brief Perform dry composition a formatted string - only calculate resulting size
 * @param *format Format string
 * @param ... Additional arguments
 * @return Count of characters in the resulting string
 * @note There is no destination buffer. This function is used to get resulting string size
*/
PRINTF_LIKE(1, 2)
int RtlSprintDry(const char * restrict format, ...);

/**
 * @brief Perform dry composition a formatted string - only calculate resulting size
 * @param *format Format string
 * @param args List of arguments
 * @return ount of characters in the resulting string
 * @note There is no destination buffer. This function is used to get resulting string size
*/
int RtlSprintDryV(const char * restrict format, va_list args);

#ifndef DISABLE_KERNEL_STDLIB

/**
 * @brief Compose a formatted string
 * @param *format Format string
 * @param ... Additional arguments
 * @return Count of characters written
*/
#define sprintf(str, ...) RtlSprint(str, __VA_ARGS__)

/**
 * @brief Compose a formatted string with length limit
 * @param *format Format string
 * @param n Length limit including terminator character
 * @param ... Additional arguments
 * @return Count of characters written
*/
#define snprintf(str, n, ...) RtlSprintN(str, n, __VA_ARGS__)

#endif

END_EXPORT_API

/**
 * @}
 */

#endif