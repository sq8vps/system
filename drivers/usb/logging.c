#include "logging.h"

struct IoSyslogHandle *UhciLogHandle = NULL;

void UhciLoggingInit(void)
{
    UhciLogHandle = IoOpenSyslog(DRIVER_NAME, SYSLOG_OUTPUT_DEFAULT);
}