#include "kernel.h"
#include "logging.h"
#include "device.h"
#include "utils.h"
#include "bridge.h"

static char driverName[] = "Generic PCI bus driver";
static char driverVendor[] = "Standard drivers";
static char driverVersion[] = "1.0.0";

static STATUS DriverDispatch(struct IoRp *rp)
{
    STATUS status;

    struct IoDeviceObject *dev = IoGetCurrentRpPosition(rp);

    switch(rp->code)
    {
        case IO_RP_ENUMERATE:
            if(NULL != ((struct PciDeviceData*)dev->privateData)->thisBridge)
                status = PciEnumerate(dev->driverObject, dev, ((struct PciDeviceData*)dev->privateData)->thisBridge);
            else
                status = IO_RP_PROCESSING_FAILED;
            break;
        case IO_RP_GET_CONFIG_SPACE:
            if(NULL != dev->privateData)
            {
                struct PciDeviceData *info = dev->privateData;
                status = PciReadConfigurationSpace(info->address, rp);
                break;
            }
            status = IO_RP_PROCESSING_FAILED;
            break;
        case IO_RP_SET_CONFIG_SPACE:
            if(NULL != dev->privateData)
            {
                struct PciDeviceData *info = dev->privateData;
                status = PciWriteConfigurationSpace(info->address, rp);
                break;
            }
            status = IO_RP_PROCESSING_FAILED;
            break;
        case IO_RP_GET_DEVICE_LOCATION:
            if(NULL != dev->privateData)
            {
                struct PciDeviceData *info = dev->privateData;
                rp->payload.location.id = info->address;
                status = OK;
                break;
            }
            status = IO_RP_PROCESSING_FAILED;
            break;
        case IO_RP_GET_DEVICE_RESOURCES:
            status = PciGetResources(rp);
            break;
        case IO_RP_GET_DEVICE_ID:
            status = PciGetSystemDeviceId(rp);
            break;
        default:
            break;
    }

    rp->status = status;
    IoFinalizeRp(rp);
    return status;
}

static STATUS DriverInit(struct ExDriverObject *driverObject)
{
    return OK;
} 

static STATUS DriverAddDevice(struct ExDriverObject *driverObject, struct IoDeviceObject *baseDeviceObject)
{
    return PciAddDevice(driverObject, baseDeviceObject);
}

STATUS DRIVER_ENTRY(struct ExDriverObject *driverObject)
{
    driverObject->init = DriverInit;
    driverObject->dispatch = DriverDispatch;
    driverObject->addDevice = DriverAddDevice;
    driverObject->name = driverName;
    driverObject->vendor = driverVendor;
    driverObject->version = driverVersion;
    PciLoggingInit();
    return OK;
}

