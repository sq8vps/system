#include "logging.h"

struct IoSyslogHandle *TtyLogHandle = NULL;

void TtyLoggingInit(void)
{
    TtyLogHandle = IoOpenSyslog(DRIVER_NAME, SYSLOG_OUTPUT_DEFAULT);
}