#include "logging.h"

struct IoSyslogHandle *I8042LogHandle = NULL;

void I8042LoggingInit(void)
{
    I8042LogHandle = IoOpenSyslog(DRIVER_NAME, SYSLOG_OUTPUT_DEFAULT);
}