#ifndef IDE_LOGGING_H_
#define IDE_LOGGING_H_

#include "io/log/syslog.h"

extern struct IoSyslogHandle *VgaLogHandle;

#define LOG(type, ...) IoWriteSyslog(VgaLogHandle, type, __VA_ARGS__)

void VgaLoggingInit(void);

#endif