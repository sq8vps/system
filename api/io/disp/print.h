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
extern int IoPrint(const char *format, ...);

/**
 * @brief Print formatted output to currently active output only in debug mode
 * @param *format Format string
 * @param ... Additional arguments
 * @return Count of characters written
 * @attention This function does not guarantee any output. The output is written to currently available and active
 * console output. Drivers should use \a IoWriteSyslog() instead.
*/
__attribute__ ((format (printf, 1, 2)))
extern int IoPrintDebug(const char *format, ...);

/**
 * @brief Print formatted output to currently active output
 * @param *format Format string
 * @param args List of additional arguments
 * @return Count of characters written
 * @attention This function does not guarantee any output. The output is written to currently available and active
 * console output. Drivers should use \a IoWriteSyslog() instead.
*/
extern int IoVprint(const char *format, va_list args);

/**
 * @brief Print formatted output to currently active output only in debug mode
 * @param *format Format string
 * @param args List of additional arguments
 * @return Count of characters written
 * @attention This function does not guarantee any output. The output is written to currently available and active
 * console output. Drivers should use \a IoWriteSyslog() instead.
*/
extern int IoVprintDebug(const char *format, va_list args);


#ifdef __cplusplus
}
#endif

#endif