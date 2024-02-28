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
static STATUS DriverDispatch(struct IoRp *rp)
{
    STATUS status;

    struct IoDeviceObject *dev = IoGetCurrentRpPosition(rp);

    switch(rp->code)
    {
        //enumerate drives connected to a controller
        case IO_RP_ENUMERATE:
            if((NULL != dev->privateData) 
                && (IDE_SUBDEVICE_CONTROLLER == ((struct IdeControllerData*)dev->privateData)->type))
            {
                if(OK == (status = IdeDetectAllDrives(dev->privateData)))
                {
                    status = IdeCreateAllDriveDevices(dev->privateData, dev, dev->driverObject);
                }
                rp->status = status;
                IoFinalizeRp(rp);
                return status;
            }
            break;
        //read or write
        case IO_RP_READ:
        case IO_RP_WRITE:
            if((NULL != dev->privateData)
                && (IDE_SUBDEVICE_DRIVE == ((struct IdeDriveData*)dev->privateData)->type))
            {
                struct IdeDriveData *info = dev->privateData;
                return IoStartRp(info->controller->channel[info->channel].queue, rp, NULL);
            }
            break;
        case IO_RP_GET_DEVICE_ID:
            status = IdeGetDeviceId(rp);
            IoFinalizeRp(rp);
            return status;
            break;
        default:
    }

    return IoSendRpDown(rp);
}

static STATUS DriverInit(struct ExDriverObject *driverObject)
{
    return OK;
} 

/**
 * @brief Add IDE controller device object
 * 
 * This function is called on the IDE controller object to create a main device.
 * It is not called for disk devices.
*/
static STATUS DriverAddDevice(struct ExDriverObject *driverObject, struct IoDeviceObject *baseDeviceObject)
{
    struct IoDeviceObject *device;
    STATUS status;
    if(OK != (status = IoCreateDevice(driverObject, IO_DEVICE_TYPE_STORAGE, 0, &device)))
        return status;
    
    if(NULL == (device->privateData = MmAllocateKernelHeap(sizeof(struct IdeControllerData))))
        return OUT_OF_RESOURCES;
    
    CmMemset(device->privateData, 0, sizeof(struct IdeControllerData));
    
    IoAttachDevice(device, baseDeviceObject);
    baseDeviceObject->driverObject->flags = IO_DEVICE_FLAG_ENUMERATION_CAPABLE;

    struct IdeControllerData *info = device->privateData;
    CmMemset(info, 0, sizeof(*info));
    info->enumerator = baseDeviceObject;
    info->type = IDE_SUBDEVICE_CONTROLLER;
        
    return IdeConfigureController(baseDeviceObject, device, info);
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

