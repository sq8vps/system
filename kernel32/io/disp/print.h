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
 * @brief Print formatted output to currently active output only in debug mode
 * @param *format Format string
 * @param ... Additional arguments
 * @return Count of characters written
 * @attention This function does not guarantee any output. The output is written to currently available and active
 * console output. Drivers should use \a IoWriteSyslog() instead.
*/
__attribute__ ((format (printf, 1, 2)))
EXTERN int IoPrintDebug(const char *format, ...);

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
 * @brief Print formatted output to currently active output only in debug mode
 * @param *format Format string
 * @param args List of additional arguments
 * @return Count of characters written
 * @attention This function does not guarantee any output. The output is written to currently available and active
 * console output. Drivers should use \a IoWriteSyslog() instead.
*/
EXTERN int IoVprintDebug(const char *format, va_list args);

#endif