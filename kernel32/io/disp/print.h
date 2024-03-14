#ifndef KERNEL_PRINT_H_
#define KERNEL_PRINT_H_

#include "defines.h"
#include <stdarg.h>

EXPORT
/**
 * @brief Print formatted output to currently active output
 * @param *format Format string
 * @param ... Additional arguments
 * @return Count of characters written
 * @attention This function does not guarantee any output. The output is written to currently available and active
 * console output. Drivers should use \a IoWriteSyslog() instead.
*/
__attribute__ ((format (printf, 1, 2)))
EXTERN int IoPrint(const char *format, ...);

EXPORT
/**
 * @brief Print formatted output to currently active output
 * @param *format Format string
 * @param args List of additional arguments
 * @return Count of characters written
 * @attention This function does not guarantee any output. The output is written to currently available and active
 * console output. Drivers should use \a IoWriteSyslog() instead.
*/
EXTERN int IoVprint(const char *format, va_list args);

EXPORT
/**
 * @brief Compose a formatted string
 * @param *format Format string
 * @param ... Additional arguments
 * @return Count of characters written
*/
__attribute__ ((format (printf, 2, 3)))
EXTERN int IoSprint(char *s, const char *format, ...);

EXPORT
/**
 * @brief Compose a formatted string with length limit
 * @param *format Format string
 * @param n Length limit including terminator character
 * @param ... Additional arguments
 * @return Count of characters written
*/
__attribute__ ((format (printf, 3, 4)))
EXTERN int IoSprintN(char *s, size_t n, const char *format, ...);

EXPORT
#ifndef DISABLE_KERNEL_STDLIB
#define sprintf(str, ...) IoSprint(str, __VA_ARGS__)
#define snprintf(str, n, ...) IoSprintN(str, n, __VA_ARGS__)
#endif

#endif