#include "logging.h"

struct IoSyslogHandle *FatLogHandle = NULL;

void FatLoggingInit(void)
{
    FatLogHandle = IoOpenSyslog(DRIVER_NAME);
}