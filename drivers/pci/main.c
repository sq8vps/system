#include "kernel.h"
#include "logging.h"
#include "device.h"
#include "utils.h"
#include "bridge.h"

static STATUS PciDispatch(struct IoRp *rp)
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
                rp->payload.location.type = IO_BUS_TYPE_PCI;
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
            status = IO_RP_PROCESSING_FAILED;
            break;
    }

    rp->status = status;
    IoFinalizeRp(rp);
    return OK;
}

static STATUS PciInit(struct ExDriverObject *driverObject)
{
    return OK;
} 


STATUS DRIVER_ENTRY(struct ExDriverObject *driverObject)
{
    driverObject->init = PciInit;
    driverObject->dispatch = PciDispatch;
    driverObject->addDevice = PciAddDevice;
    PciLoggingInit();
    return OK;
}

