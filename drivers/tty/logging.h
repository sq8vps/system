#ifndef TTY_LOGGING_H_
#define TTY_LOGGING_H_

#include "io/log/syslog.h"

extern struct IoSyslogHandle *TtyLogHandle;

#define LOG(type, ...) IoWriteSyslog(TtyLogHandle, type, __VA_ARGS__)

void TtyLoggingInit(void);

#endif