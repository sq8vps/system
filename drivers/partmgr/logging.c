#include "logging.h"

struct IoSyslogHandle *PartmgrLogHandle = NULL;

void PartmgrLoggingInit(void)
{
    PartmgrLogHandle = IoOpenSyslog(DRIVER_NAME);
}