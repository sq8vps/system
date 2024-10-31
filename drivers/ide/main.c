#include "kernel.h"
#include "logging.h"
#include "device.h"
#include "ata.h"
#include "config.h"

/**
 * @brief Request dispatch routine
*/
static STATUS IdeDispatch(struct IoRp *rp)
{
    STATUS status = OK;

    struct IoDeviceObject *dev = IoGetCurrentRpPosition(rp);

    switch(rp->code)
    {
        //enumerate drives connected to a controller
        case IO_RP_ENUMERATE:
            if(((struct IdeDeviceData*)dev->privateData)->isController)
            {
                struct IdeControllerData *info = &(((struct IdeDeviceData*)dev->privateData)->controller);
                if(OK == (status = IdeDetectAllDrives(info)))
                {
                    status = IdeCreateAllDriveDevices(info, dev, dev->driverObject);
                }
            }
            break;
        //read or write
        case IO_RP_READ:
        case IO_RP_WRITE:
            if(!((struct IdeDeviceData*)dev->privateData)->isController)
            {
                struct IdeDriveData *info = &(((struct IdeDeviceData*)dev->privateData)->drive);
                IoMarkRpPending(rp);
                return IoStartRp(info->controller->channel[info->channel].queue, rp, NULL);
            }
            break;
        case IO_RP_GET_DEVICE_ID:
            IdeGetDeviceId(rp);
            break;
        case IO_RP_STORAGE_CONTROL:
            IdeStorageControl(rp);
            break;
        default:
            rp->status = RP_PROCESSING_FAILED;
            IoFinalizeRp(rp);
            break;
    }
    return OK;
}

static STATUS IdeInit(struct ExDriverObject *driverObject)
{
    return OK;
} 

/**
 * @brief Add IDE controller device object (MDO)
 * 
 * This function is called on the IDE controller object to create a main device.
 * It is not called for disk devices.
*/
static STATUS IdeAddDevice(struct ExDriverObject *driverObject, struct IoDeviceObject *baseDeviceObject)
{
    struct IoDeviceObject *device;
    STATUS status;
    if(OK != (status = IoCreateDevice(driverObject, IO_DEVICE_TYPE_STORAGE, 0, &device)))
        return status;
    
    if(NULL == (device->privateData = MmAllocateKernelHeap(sizeof(struct IdeDeviceData))))
    {
        IoDestroyDevice(device);
        return OUT_OF_RESOURCES;
    }
    
    RtlMemset(device->privateData, 0, sizeof(struct IdeDeviceData));
    
    IoAttachDevice(device, baseDeviceObject);
    baseDeviceObject->driverObject->flags = IO_DEVICE_FLAG_ENUMERATION_CAPABLE;

    ((struct IdeDeviceData*)device->privateData)->isController = 1;
    
    struct IdeControllerData *info = &(((struct IdeDeviceData*)device->privateData)->controller);
    RtlMemset(info, 0, sizeof(*info));
    info->enumerator = baseDeviceObject;
        
    return IdeConfigureController(baseDeviceObject, device, info);
}

/**
 * @brief Main driver entry routine, called only once when the driver is loaded to the memory
*/
STATUS DRIVER_ENTRY(struct ExDriverObject *driverObject)
{
    driverObject->init = IdeInit;
    driverObject->dispatch = IdeDispatch;
    driverObject->addDevice = IdeAddDevice;
    IdeLoggingInit();
    return OK;
}

