//This header file is generated automatically
#ifndef EXPORTED___API__IO_LOG_SYSLOG_H_
#define EXPORTED___API__IO_LOG_SYSLOG_H_

#ifdef __cplusplus
extern "C" 
{
#endif

#include <stdint.h>
#include "defines.h"
#include <stdarg.h>

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
 * @brief Syslog object handle
*/
struct IoSyslogHandle
{
    uint32_t objectType;
    struct IoFileHandle *file;
    char *name;
};


/**
 * @brief Open syslog
 * @param *name Module name to be used in syslog
 * @return Syslog object handle or NULL on failure
*/
struct IoSyslogHandle* IoOpenSyslog(const char *name);


/**
 * @brief Close syslog
 * @param *handle Syslog handle
*/
void IoCloseSyslog(struct IoSyslogHandle *handle);


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


#ifdef __cplusplus
}
#endif

#endif