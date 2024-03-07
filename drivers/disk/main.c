#include "kernel.h"
#include "logging.h"
#include "disk.h"

static STATUS DiskDispatch(struct IoRp *rp)
{
    if(IoGetCurrentRpPosition(rp) == rp->device)
    {
        switch(rp->code)
        {
            case IO_RP_READ:
            case IO_RP_WRITE:
                return DiskReadWrite(rp);
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
static STATUS DiskAddDevice(struct ExDriverObject *driverObject, struct IoDeviceObject *baseDeviceObject)
{
    struct IoDeviceObject *device;
    STATUS status;
    if(OK != (status = IoCreateDevice(driverObject, IO_DEVICE_TYPE_DISK, 0, &device)))
        return status;
    
    if(NULL == (device->privateData = MmAllocateKernelHeap(sizeof(struct DiskData))))
        return OUT_OF_RESOURCES;
    
    CmMemset(device->privateData, 0, sizeof(struct DiskData));
    
    IoAttachDevice(device, baseDeviceObject);
    baseDeviceObject->driverObject->flags = 0;

    struct DiskData *info = device->privateData;
    info->usable = true;
    info->bdo = baseDeviceObject;
        
    return OK;
}

STATUS DRIVER_ENTRY(struct ExDriverObject *driverObject)
{
    driverObject->init = DiskInit;
    driverObject->dispatch = DiskDispatch;
    driverObject->addDevice = DiskAddDevice;
    DiskLoggingInit();
    return OK;
}

