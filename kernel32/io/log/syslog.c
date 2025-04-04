#include "syslog.h"
#include "mm/heap.h"
#include "rtl/stdio.h"
#include "rtl/string.h"
#include "io/fs/fs.h"
#include "mm/heap.h"
#include "io/fs/fs.h"
#include "hal/video.h"

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
    if(unlikely(NULL == h))
        return NULL_POINTER_GIVEN;

    if(HalVideoIsAvailable())
    {
        HalVideoPrint("[%s] ", h->name);

        switch(type)
        {
            case SYSLOG_INFO:
                HalVideoPrint("INFO: ");
                break;
            case SYSLOG_WARNING:
                HalVideoPrint("WARNING: ");
                break;
            case SYSLOG_ERROR:
                HalVideoPrint("ERROR: ");
                break;
            default:
                break;
        }
        HalVideoPrintV(format, args);
        HalVideoPrintChar('\n');
    }

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