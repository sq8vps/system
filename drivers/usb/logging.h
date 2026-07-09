#ifndef UHCI_LOGGING_H_
#define UHCI_LOGGING_H_

#include "io/log/syslog.h"

extern struct IoSyslogHandle *UhciLogHandle;

#define LOG(type, ...) IoWriteSyslog(UhciLogHandle, type, __VA_ARGS__)

void UhciLoggingInit(void);

#endif