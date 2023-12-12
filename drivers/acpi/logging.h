#ifndef ACPI_LOGGING_H_
#define ACPI_LOGGING_H_

#include "logging.h"
#include "kernel.h"

extern struct IoSyslogHandle *AcpiLogHandle;

void AcpiLoggingInit(void);

#endif