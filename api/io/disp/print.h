//This header file is generated automatically
#ifndef EXPORTED___API__IO_DISP_PRINT_H_
#define EXPORTED___API__IO_DISP_PRINT_H_

#ifdef __cplusplus
extern "C" 
{
#endif

#include "defines.h"
#include <stdarg.h>

/**
 * @brief Print formatted output to currently active output
 * @param *format Format string
 * @param ... Additional arguments
 * @return Count of characters written
 * @attention This function does not guarantee any output. The output is written to currently available and active
 * console output. Drivers should use \a IoWriteSyslog() instead.
*/
__attribute__ ((format (printf, 1, 2)))
int IoPrint(const char *format, ...);


/**
 * @brief Print formatted output to currently active output
 * @param *format Format string
 * @param args List of additional arguments
 * @return Count of characters written
 * @attention This function does not guarantee any output. The output is written to currently available and active
 * console output. Drivers should use \a IoWriteSyslog() instead.
*/
int IoVprint(const char *format, va_list args);


/**
 * @brief Compose a formatted string
 * @param *format Format string
 * @param ... Additional arguments
 * @return Count of characters written
*/
__attribute__ ((format (printf, 2, 3)))
int IoSprint(char *s, const char *format, ...);


/**
 * @brief Compose a formatted string with length limit
 * @param *format Format string
 * @param n Length limit including terminator character
 * @param ... Additional arguments
 * @return Count of characters written
*/
__attribute__ ((format (printf, 3, 4)))
int IoSprintN(char *s, size_t n, const char *format, ...);


#ifndef DISABLE_KERNEL_STDLIB
#define sprintf(str, ...) IoSprint(str, __VA_ARGS__)
#define snprintf(str, n, ...) IoSprintN(str, n, __VA_ARGS__)
#endif


#ifdef __cplusplus
}
#endif

#endif