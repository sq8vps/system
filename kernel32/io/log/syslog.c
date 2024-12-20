#include "syslog.h"
#include "mm/heap.h"
#include "rtl/stdio.h"
#include "rtl/string.h"
#include "io/fs/fs.h"
#include "mm/heap.h"

struct IoSyslogHandle IoKernelLog = {.output = SYSLOG_OUTPUT_MAIN, .name = "Kernel"};

struct IoSyslogHandle* IoOpenSyslog(const char *name, enum IoSyslogOutput output)
{
    UNUSED(output);
    struct IoSyslogHandle *h = ObCreateKernelObjectEx(OB_SYSLOG, RtlStrlen(name) + 1);
    if(NULL == h)
        return NULL;
    
    RtlStrcpy(h->name, name);
    return h;
}

void IoCloseSyslog(struct IoSyslogHandle *handle)
{
    ObDestroyObject(handle);
}

STATUS IoWriteSyslogV(struct IoSyslogHandle *h, enum IoSyslogMessageType type, const char *format, va_list args)
{
    if(NULL == h)
        return NULL_POINTER_GIVEN;

    RtlPrint("[%s] ", h->name);

    switch(type)
    {
        case SYSLOG_INFO:
            RtlPrint("INFO: ");
            break;
        case SYSLOG_WARNING:
            RtlPrint("WARNING: ");
            break;
        case SYSLOG_ERROR:
            RtlPrint("ERROR: ");
            break;
        default:
            break;
    }
    RtlVprint(format, args);
    RtlPrint("\n");
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