#include "syslog.h"
#include "mm/heap.h"
#include "rtl/stdio.h"
#include "rtl/string.h"
#include "io/fs/fs.h"
#include "mm/heap.h"
#include "io/fs/fs.h"
#include "hal/video.h"
#include "hal/time.h"


#include "hal/debug.h"

#define PRINT_STR HalVideoPrint
#define PRINT_CHAR HalVideoPrintChar

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
        return BAD_PARAMETER;

    if(HalVideoIsAvailable())
    {
        char buffer[64];
        uint64_t timestamp = HalGetTimestampMillis();
        snprintf(buffer, sizeof(buffer), "[%llu.%04llu] ", timestamp / 1000, timestamp % 1000);
        PRINT_STR(buffer);
        PRINT_STR(h->name);

        switch(type)
        {
            case SYSLOG_INFO:
                PRINT_STR(" (INFO): ");
                break;
            case SYSLOG_WARNING:
                PRINT_STR(" (WARNING): ");
                break;
            case SYSLOG_ERROR:
                PRINT_STR(" (ERROR): ");
                break;
            default:
                break;
        }

        bool isFormatted = false;
        const char *s = format;
        while('\0' != *s)
        {
            if('%' == *s)
            {
                isFormatted = true;
                break;
            }
            ++s;
        }

        if(!isFormatted)
        {
            PRINT_STR(format);
        }
        else
        {
            int size = RtlSprintDryV(format, args);
            if(size <= 0)
                return BAD_PARAMETER;
            char *buffer = MmAllocateKernelHeap(size + 1);
            if(NULL == buffer)
                return OUT_OF_RESOURCES;
            RtlSprintV(buffer, format, args);
            PRINT_STR(buffer);
            MmFreeKernelHeap(buffer);
        }
        PRINT_CHAR('\n');
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