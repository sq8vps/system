#include "logging.h"

struct IoSyslogHandle *IdeLogHandle = NULL;

void IdeLoggingInit(void)
{
    IdeLogHandle = IoOpenSyslog(DRIVER_NAME);
}