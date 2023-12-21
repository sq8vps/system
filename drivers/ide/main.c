#include "kernel.h"
#include "logging.h"
#include "device.h"

static char driverName[] = "Generic PCI IDE storage driver";
static char driverVendor[] = "Standard drivers";
static char driverVersion[] = "1.0.0";

static STATUS DriverDispatch(struct IoDriverRp *rp)
{
    STATUS status;
    switch(rp->code)
    {
        case IO_RP_ENUMERATE:
            if(NULL != rp->device->privateData)
            {
                if(OK == (status = IdeDetectAllDrives(rp->device->privateData)))
                {
                    status = IdeCreateAllDriveDevices(rp->device->privateData, rp->device->driverObject);
                }
            }
            else
                goto DriverDispatchForwardRp;
            break;
        default:
            goto DriverDispatchForwardRp;
            break;
    }
    rp->status = status;
    IoFinalizeRp(rp);
    return status;
DriverDispatchForwardRp:
    return IoSendRpDown(rp->device, rp);
}

static STATUS DriverInit(struct ExDriverObject *driverObject)
{
    return OK;
} 

static STATUS DriverAddDevice(struct ExDriverObject *driverObject, struct IoSubDeviceObject *baseDeviceObject)
{
    struct IoSubDeviceObject *device;
    STATUS status;
    if(OK != (status = IoCreateSubDevice(driverObject, 0, &device)))
        return status;
    
    if(NULL == (device->privateData = MmAllocateKernelHeap(sizeof(struct IdeDeviceData))))
        return OUT_OF_RESOURCES;
    
    CmMemset(device->privateData, 0, sizeof(struct IdeDeviceData));
    
    IoAttachSubDevice(device, baseDeviceObject);
    baseDeviceObject->driverObject->flags = IO_DEVICE_FLAG_ENUMERATION_CAPABLE;

    struct IdeDeviceData *info = device->privateData;
    CmMemset(info, 0, sizeof(*info));
    info->enumerator = baseDeviceObject;
        
    return IdeConfigureController(device, info);
}

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

