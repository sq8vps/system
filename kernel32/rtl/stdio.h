#ifndef KERNEL_VPRINTF_H_
#define KERNEL_VPRITNF_H_

#include "defines.h"
#include <stdarg.h>

EXPORT_API

/**
 * @brief Compose a formatted string
 * @param *format Format string
 * @param ... Additional arguments
 * @return Count of characters written
*/
__attribute__ ((format (printf, 2, 3)))
int RtlSprint(char *s, const char *format, ...);

/**
 * @brief Compose a formatted string
 * @param *format Format string
 * @param va_list Argument list
 * @return Count of characters written
*/
int RtlSprintV(char *s, const char *format, va_list args);


/**
 * @brief Compose a formatted string with length limit
 * @param *format Format string
 * @param n Length limit including terminator character
 * @param ... Additional arguments
 * @return Count of characters written
*/
__attribute__ ((format (printf, 3, 4)))
int RtlSprintN(char *s, size_t n, const char *format, ...);

/**
 * @brief Perform dry composition a formatted string - only calculate resulting size
 * @param *format Format string
 * @param ... Additional arguments
 * @return Count of characters in the resulting string
 * @note There is no destination buffer. This function is used to get resulting string size
*/
__attribute__ ((format (printf, 1, 2)))
int RtlSprintDry(const char *format, ...);

/**
 * @brief Perform dry composition a formatted string - only calculate resulting size
 * @param *format Format string
 * @param args List of arguments
 * @return ount of characters in the resulting string
 * @note There is no destination buffer. This function is used to get resulting string size
*/
int RtlSprintDryV(const char *format, va_list args);

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

#endif