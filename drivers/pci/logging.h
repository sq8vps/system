#ifndef PCI_LOGGING_H_
#define PCI_LOGGING_H_

#include "logging.h"
#include "kernel.h"

extern struct IoSyslogHandle *PciLogHandle;

#define LOG(type, ...) IoWriteSyslog(PciLogHandle, type, __VA_ARGS__)

void PciLoggingInit(void);

#endif