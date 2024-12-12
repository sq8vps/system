#include "logging.h"
#include "device.h"
#include "io/dev/rp.h"
#include "ioctl.h"

static STATUS TtyDispatch(struct IoRp *rp)
{
    STATUS status = OK;

    struct TtyDeviceData *info = rp->device->privateData;

    switch(rp->code)
    {
        case IO_RP_OPEN:
        case IO_RP_CLOSE:
            status = OK;
            break;
        case IO_RP_WRITE:
            if(!info->activated || (NULL == info->queue.write))
                status = DEVICE_NOT_AVAILABLE;
            else
            {
                return IoStartRp(info->queue.write, rp, NULL);
            }
            break;
        case IO_RP_IOCTL:
            status = TtyHandleIoctl(rp);
            break;
        default:
            status = RP_PROCESSING_FAILED;
            break;
    }

    rp->status = status;
    IoFinalizeRp(rp);
    return OK;
}

static STATUS TtyInit(struct ExDriverObject *driverObject)
{
    return TtyCreateDevice(driverObject, TTY_TYPE_VT, NULL);
} 


STATUS DRIVER_ENTRY(struct ExDriverObject *driverObject)
{
    driverObject->init = TtyInit;
    driverObject->dispatch = TtyDispatch;
    driverObject->addDevice = NULL;
    TtyLoggingInit();
    return OK;
}

