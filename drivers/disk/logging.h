#ifndef DISK_LOGGING_H_
#define DISK_LOGGING_H_

#include "logging.h"
#include "kernel.h"

extern struct IoSyslogHandle *DiskLogHandle;

#define LOG(type, ...) IoWriteSyslog(DiskLogHandle, type, __VA_ARGS__)

void DiskLoggingInit(void);

#endif