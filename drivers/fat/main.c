#include "kernel.h"
#include "mount.h"
#include "logging.h"
#include "fsctrl.h"
#include "read.h"

static STATUS FatDispatch(struct IoRp *rp)
{
    if(IoGetCurrentRpPosition(rp) == rp->device)
    {
        switch(rp->code)
        {
            //handle filesystem requests
            case IO_RP_FILESYSTEM_CONTROL:
                return FatFsControl(rp);
                break;
            //handle open and close request
            case IO_RP_OPEN:
            case IO_RP_CLOSE:
                //currently do nothing, just finalize
                rp->status = OK;
                IoFinalizeRp(rp);
                return OK;
                break;
            //handle read and write requests
            case IO_RP_READ:
            case IO_RP_WRITE:
                return FatReadWrite(rp);
                break;
            default:
                rp->status = IO_RP_CODE_UNKNOWN;
                IoFinalizeRp(rp);
                return IO_RP_CODE_UNKNOWN;
                break;
        }
    }
    return IO_RP_PROCESSING_FAILED;
}

static STATUS FatInit(struct ExDriverObject *driverObject)
{
    return OK;
} 


static STATUS FatAddDevice(struct ExDriverObject *driverObject, struct IoDeviceObject *baseDeviceObject)
{
    return OK;
}

STATUS DRIVER_ENTRY(struct ExDriverObject *driverObject)
{
    driverObject->init = FatInit;
    driverObject->dispatch = FatDispatch;
    driverObject->addDevice = FatAddDevice;
    driverObject->mount = FatMount;
    FatLoggingInit();

    return OK;
}

