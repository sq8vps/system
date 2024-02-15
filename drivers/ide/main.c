#include "kernel.h"
#include "logging.h"
#include "device.h"
#include "ata.h"
#include "config.h"

static char driverName[] = "Generic PCI IDE storage driver";
static char driverVendor[] = "Standard drivers";
static char driverVersion[] = "1.0.0";

/**
 * @brief Request dispatch routine
*/
static STATUS DriverDispatch(struct IoDriverRp *rp)
{
    STATUS status;
    if(IoGetCurrentRpPosition(rp) == rp->device)
    {
        switch(rp->code)
        {
            //enumerate drives connected to a controller
            case IO_RP_ENUMERATE:
                if((NULL != rp->device->privateData) 
                    && (IDE_SUBDEVICE_CONTROLLER == ((struct IdeControllerData*)rp->device->privateData)->type))
                {
                    if(OK == (status = IdeDetectAllDrives(rp->device->privateData)))
                    {
                        status = IdeCreateAllDriveDevices(rp->device->privateData, rp->device->driverObject);
                    }
                    rp->status = status;
                    IoFinalizeRp(rp);
                    return status;
                }
                break;
            //read or write
            case IO_RP_READ:
            case IO_RP_WRITE:
                if((NULL != rp->device->privateData)
                    && (IDE_SUBDEVICE_DRIVE == ((struct IdeDriveData*)rp->device->privateData)->type))
                {
                    struct IdeDriveData *info = rp->device->privateData;
                    return IoStartRp(info->controller->channel[info->channel].queue, rp, NULL);
                }
                break;
            //get drive access parameters: block size, alignment etc.
            case IO_RP_GET_IO_PARAMETERS:
                if(NULL != rp->device->privateData)
                {
                    CmMemset(&(rp->payload.ioParams), 0, sizeof(rp->payload.ioParams));
                    if(IDE_SUBDEVICE_CONTROLLER == ((struct IdeControllerData*)rp->device->privateData)->type)
                    {
                        //cannot read from or write to the controller itself
                        rp->status = OK;
                        IoFinalizeRp(rp);
                        return OK;
                    }
                    else if(IDE_SUBDEVICE_DRIVE == ((struct IdeDriveData*)rp->device->privateData)->type)
                    {
                        struct IdeDriveData *info = (struct IdeDriveData*)rp->device->privateData;
                        if(info->present && info->usable)
                        {
                            rp->payload.ioParams.read.direct.available = true;
                            rp->payload.ioParams.read.direct.blockSize = info->sectorSize;
                            rp->payload.ioParams.read.direct.minOffset = 0;
                            rp->payload.ioParams.read.direct.maxOffset = info->sectors * info->sectorSize - 1;
                            rp->payload.ioParams.read.direct.requiredAlignment = 2; //16-bit alignment is required for DMA
                            //read and write is symmetrical
                            rp->payload.ioParams.write = rp->payload.ioParams.read;
                        }
                        rp->status = OK;
                        IoFinalizeRp(rp);
                        return OK;
                    }
                }   
                break;
            default:
        }
    }
    return IoSendRpDown(rp);
}

static STATUS DriverInit(struct ExDriverObject *driverObject)
{
    return OK;
} 

/**
 * @brief Add IDE controller subdevice object
 * 
 * This function is called on the IDE controller object to create a main subdevice.
 * It is not called for disk devices.
*/
static STATUS DriverAddDevice(struct ExDriverObject *driverObject, struct IoSubDeviceObject *baseDeviceObject)
{
    struct IoSubDeviceObject *device;
    STATUS status;
    if(OK != (status = IoCreateSubDevice(driverObject, 0, &device)))
        return status;
    
    if(NULL == (device->privateData = MmAllocateKernelHeap(sizeof(struct IdeControllerData))))
        return OUT_OF_RESOURCES;
    
    CmMemset(device->privateData, 0, sizeof(struct IdeControllerData));
    
    IoAttachSubDevice(device, baseDeviceObject);
    baseDeviceObject->driverObject->flags = IO_DEVICE_FLAG_ENUMERATION_CAPABLE;

    struct IdeControllerData *info = device->privateData;
    CmMemset(info, 0, sizeof(*info));
    info->enumerator = baseDeviceObject;
    info->type = IDE_SUBDEVICE_CONTROLLER;
        
    return IdeConfigureController(device, info);
}

/**
 * @brief Main driver entry routine, called only once when the driver is loaded to the memory
*/
STATUS DRIVER_ENTRY(struct ExDriverObject *driverObject)
{
    driverObject->init = DriverInit;
    driverObject->dispatch = DriverDispatch;
    driverObject->addDevice = DriverAddDevice;
    driverObject->name = driverName;
    driverObject->vendor = driverVendor;
    driverObject->version = driverVersion;
    IdeLoggingInit();
    return OK;
}

