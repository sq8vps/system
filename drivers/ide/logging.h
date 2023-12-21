#ifndef IDE_LOGGING_H_
#define IDE_LOGGING_H_

#include "logging.h"
#include "kernel.h"

extern struct IoSyslogHandle *IdeLogHandle;

#define LOG(type, ...) IoWriteSyslog(IdeLogHandle, type, __VA_ARGS__)

void IdeLoggingInit(void);

#endif