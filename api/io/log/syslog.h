//This header file is generated automatically
#ifndef EXPORTED___API__IO_LOG_SYSLOG_H_
#define EXPORTED___API__IO_LOG_SYSLOG_H_

#ifdef __cplusplus
extern "C" 
{
#endif

#include <stdint.h>
#include "defines.h"
#include "io/fs/fs.h"
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
    struct IoFileHandle *file;
    char *name;
};

/**
 * @brief Open syslog
 * @param *name Module name to be used in syslog
 * @return Syslog object handle or NULL on failure
*/
extern struct IoSyslogHandle* IoOpenSyslog(const char *name);

/**
 * @brief Close syslog
 * @param *handle Syslog handle
*/
extern void IoCloseSyslog(struct IoSyslogHandle *handle);

/**
 * @brief Write to syslog
 * @param *h Syslog handle
 * @param type Message type
 * @param *format Format specifier (printf-like)
 * @param args Additional arguments
 * @return Status code
*/
extern STATUS IoWriteSyslogV(struct IoSyslogHandle *h, enum IoSyslogMessageType type, const char *format, va_list args);

/**
 * @brief Write to syslog
 * @param *h Syslog handle
 * @param type Message type
 * @param *format Format specifier (printf-like)
 * @param ... Additional arguments
 * @return Status code
*/
extern STATUS IoWriteSyslog(struct IoSyslogHandle *h, enum IoSyslogMessageType type, const char *format, ...);


#ifdef __cplusplus
}
#endif

#endif