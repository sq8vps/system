#include "syslog.h"
#include "mm/heap.h"
#include "io/disp/print.h"

struct IoSyslogHandle* IoOpenSyslog(const char *name)
{
    struct IoSyslogHandle *h = MmAllocateKernelHeap(sizeof(struct IoSyslogHandle));
    if(NULL == h)
        return NULL;
    
    h->name = (char*)name;
    return h;
}

void IoCloseSyslog(struct IoSyslogHandle *handle)
{
    MmFreeKernelHeap(handle);
}

STATUS IoWriteSyslogV(struct IoSyslogHandle *h, enum IoSyslogMessageType type, const char *format, va_list args)
{
    if(NULL == h)
        return NULL_POINTER_GIVEN;

    IoPrint("[%s] ", h->name);

    switch(type)
    {
        case SYSLOG_INFO:
            IoPrint("INFO: ");
            break;
        case SYSLOG_WARNING:
            IoPrint("WARNING: ");
            break;
        case SYSLOG_ERROR:
            IoPrint("ERROR: ");
            break;
        default:
            break;
    }
    IoVprint(format, args);
    IoPrint("\n");
    return OK;
}

STATUS IoWriteSyslog(struct IoSyslogHandle *h, enum IoSyslogMessageType type, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    STATUS ret = IoWriteSyslogV(h, type, format, args);
    va_end(args);
    return ret;
}