#include "kernel.h"
#include "logging.h"
#include "part.h"

static char driverName[] = "Partition manager";
static char driverVendor[] = "Standard drivers";
static char driverVersion[] = "1.0.0";

static STATUS PartmgrDispatch(struct IoDriverRp *rp)
{
    if(IoGetCurrentRpPosition(rp) == rp->device)
    {
        switch(rp->code)
        {
            case IO_RP_READ:
            case IO_RP_WRITE:
                //return DiskReadWrite(rp);
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

static STATUS PartmgrInit(struct ExDriverObject *driverObject)
{
    return OK;
} 

/**
 * @brief Add partition manager subdevice object
 * 
 * This function is called when a disk device is created to instantiate a partition manager for given drive
*/
static STATUS PartmgrAddDevice(struct ExDriverObject *driverObject, struct IoSubDeviceObject *baseDeviceObject)
{
    struct IoSubDeviceObject *device;
    STATUS status;
    if(OK != (status = IoCreateSubDevice(driverObject, 0, &device)))
        return status;
    
    if(NULL == (device->privateData = MmAllocateKernelHeap(sizeof(struct PartmgrDriveData))))
        return OUT_OF_RESOURCES;
    
    CmMemset(device->privateData, 0, sizeof(struct PartmgrDriveData));
    
    IoAttachSubDevice(device, baseDeviceObject);
    baseDeviceObject->driverObject->flags = 0;

    struct PartmgrDriveData *info = device->privateData;
    info->scheme = SCHEME_NONE;
    info->usable = false;
    info->device = device;
    
    struct IoDriverRp *rp;
    status = IoCreateRp(&rp);
    if(OK != status)
        return status;
    
    rp->device = baseDeviceObject;
    rp->code = IO_RP_GET_IO_PARAMETERS;
    rp->flags = IO_DRIVER_RP_FLAG_SYNCHRONOUS;
    status = IoSendRp(baseDeviceObject, NULL, rp);
    if(OK == status)
    {
        info->usable = true;
        info->ioParams = rp->payload.ioParams;
        
        PartmgrInitialize(info);
    }

    MmFreeKernelHeap(rp);
        
    return status;
}

STATUS DRIVER_ENTRY(struct ExDriverObject *driverObject)
{
    driverObject->init = PartmgrInit;
    driverObject->dispatch = PartmgrDispatch;
    driverObject->addDevice = PartmgrAddDevice;
    driverObject->name = driverName;
    driverObject->vendor = driverVendor;
    driverObject->version = driverVersion;
    PartmgrLoggingInit();
    return OK;
}

