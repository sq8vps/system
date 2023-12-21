#ifndef ACPI_LOGGING_H_
#define ACPI_LOGGING_H_

#include "logging.h"
#include "kernel.h"

extern struct IoSyslogHandle *AcpiLogHandle;

#define LOG(type, ...) IoWriteSyslog(AcpiLogHandle, type, __VA_ARGS__)

void AcpiLoggingInit(void);

#endif