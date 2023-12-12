#include "logging.h"

struct IoSyslogHandle *PciLogHandle = NULL;

void PciLoggingInit(void)
{
    PciLogHandle = IoOpenSyslog(DRIVER_NAME);
}