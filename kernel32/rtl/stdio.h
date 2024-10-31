#ifndef KERNEL_VPRINTF_H_
#define KERNEL_VPRITNF_H_

#include "defines.h"
#include <stdarg.h>

EXPORT_API

/**
 * @brief Print formatted output to currently active output
 * @param *format Format string
 * @param ... Additional arguments
 * @return Count of characters written
 * @attention This function does not guarantee any output. The output is written to currently available and active
 * console output. Drivers should use \a IoWriteSyslog() instead.
*/
__attribute__ ((format (printf, 1, 2)))
int RtlPrint(const char *format, ...);


/**
 * @brief Print formatted output to currently active output
 * @param *format Format string
 * @param args List of additional arguments
 * @return Count of characters written
 * @attention This function does not guarantee any output. The output is written to currently available and active
 * console output. Drivers should use \a IoWriteSyslog() instead.
*/
int RtlVprint(const char *format, va_list args);


/**
 * @brief Compose a formatted string
 * @param *format Format string
 * @param ... Additional arguments
 * @return Count of characters written
*/
__attribute__ ((format (printf, 2, 3)))
int RtlSprint(char *s, const char *format, ...);


/**
 * @brief Compose a formatted string with length limit
 * @param *format Format string
 * @param n Length limit including terminator character
 * @param ... Additional arguments
 * @return Count of characters written
*/
__attribute__ ((format (printf, 3, 4)))
int RtlSprintN(char *s, size_t n, const char *format, ...);


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