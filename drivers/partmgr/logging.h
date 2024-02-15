#ifndef LOGGING_H_
#define LOGGING_H_

#include "logging.h"
#include "kernel.h"

extern struct IoSyslogHandle *PartmgrLogHandle;

#define LOG(type, ...) IoWriteSyslog(PartmgrLogHandle, type, __VA_ARGS__)

void PartmgrLoggingInit(void);

#endif