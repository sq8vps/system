#ifndef PCI_LOGGING_H_
#define PCI_LOGGING_H_

#include "logging.h"
#include "kernel.h"

extern struct IoSyslogHandle *TtyLogHandle;

#define LOG(type, ...) IoWriteSyslog(TyyLogHandle, type, __VA_ARGS__)

void TtyLoggingInit(void);

#endif