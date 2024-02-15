#include "logging.h"

struct IoSyslogHandle *DiskLogHandle = NULL;

void DiskLoggingInit(void)
{
    DiskLogHandle = IoOpenSyslog(DRIVER_NAME);
}