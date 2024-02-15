#include "kernel.h"
#include "logging.h"
#include "disk.h"

static char driverName[] = "Generic disk driver";
static char driverVendor[] = "Standard drivers";
static char driverVersion[] = "1.0.0";

static STATUS DiskDispatch(struct IoDriverRp *rp)
{
    if(IoGetCurrentRpPosition(rp) == rp->device)
    {
        switch(rp->code)
        {
            case IO_RP_READ:
            case IO_RP_WRITE:
                return DiskReadWrite(rp);
                break;
            case IO_RP_GET_IO_PARAMETERS:
                rp->device = rp->device->attachedTo;
                break;
            default:
                break;
        }
    }
    IoSendRpDown(rp);
    return OK;
}

static STATUS DiskInit(struct ExDriverObject *driverObject)
{
    return OK;
} 

/**
 * @brief Add disk subdevice object
 * 
 * This function is called on the disk drive object to create a main subdevice.
*/
static STATUS DiskAddDevice(struct ExDriverObject *driverObject, struct IoSubDeviceObject *baseDeviceObject)
{
    struct IoSubDeviceObject *device;
    STATUS status;
    if(OK != (status = IoCreateSubDevice(driverObject, 0, &device)))
        return status;
    
    if(NULL == (device->privateData = MmAllocateKernelHeap(sizeof(struct DiskData))))
        return OUT_OF_RESOURCES;
    
    CmMemset(device->privateData, 0, sizeof(struct DiskData));
    
    IoAttachSubDevice(device, baseDeviceObject);
    baseDeviceObject->driverObject->flags = 0;

    struct DiskData *info = device->privateData;
    info->usable = false;
    
    struct IoDriverRp *rp;
    status = IoCreateRp(&rp);
    if(OK != status)
        return status;
    
    rp->device = baseDeviceObject;
    rp->code = IO_RP_GET_IO_PARAMETERS;
    rp->flags = IO_DRIVER_RP_FLAG_SYNCHRONOUS;
    status = IoSendRp(baseDeviceObject, NULL, rp);
    if(OK != status)
    {
        MmFreeKernelHeap(rp);
        return status;
    }

    info->ioParams = rp->payload.ioParams;
    info->usable = true;
        
    return OK;
}

STATUS DRIVER_ENTRY(struct ExDriverObject *driverObject)
{
    driverObject->init = DiskInit;
    driverObject->dispatch = DiskDispatch;
    driverObject->addDevice = DiskAddDevice;
    driverObject->name = driverName;
    driverObject->vendor = driverVendor;
    driverObject->version = driverVersion;
    DiskLoggingInit();
    return OK;
}

