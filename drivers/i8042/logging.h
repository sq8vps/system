#ifndef PCI_LOGGING_H_
#define PCI_LOGGING_H_

#include "logging.h"
#include "io/log/syslog.h"

extern struct IoSyslogHandle *I8042LogHandle;

#define LOG(type, ...) IoWriteSyslog(I8042LogHandle, type, __VA_ARGS__)

void I8042LoggingInit(void);

#endif