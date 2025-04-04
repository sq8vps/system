#ifndef DISK_LOGGING_H_
#define DISK_LOGGING_H_

#include "io/log/syslog.h"

extern struct IoSyslogHandle *FatLogHandle;

#define LOG(type, ...) IoWriteSyslog(FatLogHandle, type, __VA_ARGS__)

void FatLoggingInit(void);

#endif