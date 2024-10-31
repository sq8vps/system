#include "logging.h"

struct IoSyslogHandle *AcpiLogHandle = NULL;

void AcpiLoggingInit(void)
{
    AcpiLogHandle = IoOpenSyslog(DRIVER_NAME, SYSLOG_OUTPUT_DEFAULT);
}