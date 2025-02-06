#ifndef TTY_LOGGING_H_
#define TTY_LOGGING_H_

#include "kernel.h"

extern struct IoSyslogHandle *TtyLogHandle;

#define LOG(type, ...) IoWriteSyslog(TyyLogHandle, type, __VA_ARGS__)

void TtyLoggingInit(void);

#endif