#include "errno.h"
#include "sys/status.h"

static char *__nabla_error_strings[] = 
{
    "Not implemented"
};

int __nabla_kernel_status_to_errno(STATUS status)
{
    status = -status;
    switch(status)
    {
        case OK:
            return -ENOERR;
        case BAD_PARAMETER:
            return -EINVAL;
        case NOT_IMPLEMENTED:
            return -ENOSYS;
        case NOT_SUPPORTED:
            return -ENOSUP;
        case OUT_OF_RESOURCES:
            return -ENOMEM;
        case DEVICE_NOT_AVAILABLE:
            return -ENODEV;
        case BAD_ALIGNMENT:
            return -EALIGN;
        case TIMEOUT:
            return -ETIMEOUT;
        case BUSY:
            return -EBUSY;
        case ALREADY_EXISTS:
            return -EEXIST;
        case CORRUPTED:
            return -ECORRUPT;
        case NOT_FOUND:
            return -ENOENT;
        case BAD_TYPE:
            return -EINVAL;
        case FILE_CLOSED:
            return -ECLOSED;
        case RESOURCE_BOUND:
            return -EBUSY;
        case READ_ONLY:
            return -EROFS;
        case OPERATION_INCOMPLETE:
            return -EINCMPLT;
        
        //these error should not be reported to userland
        case UNDEFINED_SYMBOL:
        case PAGE_NOT_PRESENT:
        case RESOURCE_PERSISTENT:
        case UNKNOWN_ERROR:
            return -EUNKNOWN;
    }
    return -EUNKNOWN;
}

char *__nabla_libc_strerror(int errno)
{
    return __nabla_error_strings[0];
}