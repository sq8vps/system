#include "logging.h"

struct IoSyslogHandle *VgaLogHandle = NULL;

void VgaLoggingInit(void)
{
    VgaLogHandle = IoOpenSyslog(DRIVER_NAME, SYSLOG_OUTPUT_DEFAULT);
}