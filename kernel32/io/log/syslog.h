#ifndef KERNEL_SYSLOG_H_
#define KERNEL_SYSLOG_H_

#include <stdint.h>
#include "defines.h"
#include <stdarg.h>
#include "ob/ob.h"

EXPORT_API

struct IoSyslogHandle;

/**
 * @brief Syslog message types
*/
enum IoSyslogMessageType
{
    SYSLOG_INFO,
    SYSLOG_WARNING,
    SYSLOG_ERROR,
};

/**
 * @brief Syslog message output stream
 */
enum IoSyslogOutput
{
    SYSLOG_OUTPUT_MAIN = 0, /**< Main/kernel system log */
    SYSLOG_OUTPUT_OWN, /**< Own system log in separate file */
    SYSLOG_OUTPUT_DEFAULT = SYSLOG_OUTPUT_MAIN, /**< Default system log */
};

/**
 * @brief Open syslog
 * @param *name Module name to be used in syslog
 * @param output Syslog output stream
 * @return Syslog object handle or NULL on failure
*/
struct IoSyslogHandle* IoOpenSyslog(const char *name, enum IoSyslogOutput output);

/**
 * @brief Close syslog
 * @param *h Syslog handle
*/
void IoCloseSyslog(struct IoSyslogHandle *h);


/**
 * @brief Write to syslog
 * @param *h Syslog handle
 * @param type Message type
 * @param *format Format specifier (printf-like)
 * @param args Additional arguments
 * @return Status code
*/
STATUS IoWriteSyslogV(struct IoSyslogHandle *h, enum IoSyslogMessageType type, const char *format, va_list args);


/**
 * @brief Write to syslog
 * @param *h Syslog handle
 * @param type Message type
 * @param *format Format specifier (printf-like)
 * @param ... Additional arguments
 * @return Status code
*/
__attribute__ ((format (printf, 3, 4)))
STATUS IoWriteSyslog(struct IoSyslogHandle *h, enum IoSyslogMessageType type, const char *format, ...);

END_EXPORT_API

extern struct IoSyslogHandle IoKernelLog;

/**
 * @brief Write to kernel log
 * @param TYPE Message type (severity)
 * @param ... printf-like arguments
 * @return Status code
 * @warning This macro is for kernel code use only
 */
#define LOG(TYPE, ...) IoWriteSyslog(&IoKernelLog, (TYPE), __VA_ARGS__)

#endif