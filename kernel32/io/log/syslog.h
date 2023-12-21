#ifndef KERNEL_SYSLOG_H_
#define KERNEL_SYSLOG_H_

#include <stdint.h>
#include "defines.h"
#include "io/fs/fs.h"
#include <stdarg.h>

EXPORT
/**
 * @brief Syslog message types
*/
enum IoSyslogMessageType
{
    SYSLOG_INFO,
    SYSLOG_WARNING,
    SYSLOG_ERROR,
};

EXPORT
/**
 * @brief Syslog object handle
*/
struct IoSyslogHandle
{
    struct IoFileHandle *file;
    char *name;
};

EXPORT
/**
 * @brief Open syslog
 * @param *name Module name to be used in syslog
 * @return Syslog object handle or NULL on failure
*/
EXTERN struct IoSyslogHandle* IoOpenSyslog(const char *name);

EXPORT
/**
 * @brief Close syslog
 * @param *handle Syslog handle
*/
EXTERN void IoCloseSyslog(struct IoSyslogHandle *handle);

EXPORT
/**
 * @brief Write to syslog
 * @param *h Syslog handle
 * @param type Message type
 * @param *format Format specifier (printf-like)
 * @param args Additional arguments
 * @return Status code
*/
EXTERN STATUS IoWriteSyslogV(struct IoSyslogHandle *h, enum IoSyslogMessageType type, const char *format, va_list args);

EXPORT
/**
 * @brief Write to syslog
 * @param *h Syslog handle
 * @param type Message type
 * @param *format Format specifier (printf-like)
 * @param ... Additional arguments
 * @return Status code
*/
__attribute__ ((format (printf, 3, 4)))
EXTERN STATUS IoWriteSyslog(struct IoSyslogHandle *h, enum IoSyslogMessageType type, const char *format, ...);

#endif