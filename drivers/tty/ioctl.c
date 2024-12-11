#include "ioctl.h"
#include "io/dev/rp.h"
#include "device.h"
#include "io/dev/dev.h"

STATUS TtyHandleIoctl(struct IoRp *rp)
{
    STATUS status = OK;

    if(IO_RP_IOCTL != rp->code)
        return RP_CODE_UNKNOWN;

    switch(rp->payload.ioctl.code)
    {
        case TTY_IOCTL_NONE:
            rp->payload.ioctl.data = NULL;
            status = OK;
            break;
        case TTY_IOCTL_CREATE_VT:
            status = TtyCreateDevice(rp->device->driverObject, TTY_TYPE_VT, (uint32_t*)&(rp->payload.ioctl.data));
            break;
        case TTY_IOCTL_ACTIVATE:
            struct TtyDeviceData *info = rp->device->privateData;
            info->activated = true;
            status = OK;
            break;
        default:
            status = IOCTL_UNKNOWN;
            break;
    }
    return status;
}