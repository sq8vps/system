#include "io/dev/rp.h"
#include "ex/kdrv/kdrv.h"
#include "io/dev/dev.h"
#include "io/fs/devfs.h"

static STATUS NullDispatch(struct IoRp *rp)
{
    STATUS status = OK;

    switch(rp->code)
    {
        case IO_RP_OPEN:
        case IO_RP_CLOSE:
            status = OK;
            break;
        case IO_RP_WRITE:
            break;
        case IO_RP_READ:
            rp->size = 0;
            break;
        default:
            status = RP_PROCESSING_FAILED;
            break;
    }

    rp->status = status;
    IoFinalizeRp(rp);
    return OK;
}

static STATUS NullInit(struct ExDriverObject *driverObject)
{
    STATUS status = OK;
    struct IoDeviceObject *dev;
    status = IoCreateDevice(driverObject, IO_DEVICE_TYPE_OTHER, 
        IO_DEVICE_FLAG_HIDDEN | IO_DEVICE_FLAG_DIRECT_IO | IO_DEVICE_FLAG_BUFFERED_IO | IO_DEVICE_FLAG_STANDALONE | IO_DEVICE_FLAG_PERSISTENT, &dev);
    if(OK != status)
        return status;
    dev->blockSize = 1;
    dev->alignment = 1;
    return IoCreateDeviceFile(dev, IO_VFS_FLAG_PERSISTENT | IO_VFS_FLAG_NO_CACHE, "null");
} 


STATUS DRIVER_ENTRY(struct ExDriverObject *driverObject)
{
    driverObject->init = NullInit;
    driverObject->dispatch = NullDispatch;
    driverObject->addDevice = NULL;
    return OK;
}

