#include "kernel.h"
#include "mount.h"
#include "logging.h"

static STATUS FatDispatch(struct IoRp *rp)
{
    if(IoGetCurrentRpPosition(rp) == rp->device)
    {
        switch(rp->code)
        {
            case IO_RP_READ:
            case IO_RP_WRITE:
                
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

