#include "kernel.h"
#include "logging.h"
#include "device.h"
#include "utils.h"
#include "bridge.h"

static char driverName[] = "Generic PCI bus driver";
static char driverVendor[] = "Standard drivers";
static char driverVersion[] = "1.0.0";

static STATUS DriverDispatch(struct IoDriverRp *rp)
{
    STATUS status;
    if(IoGetCurrentRpPosition(rp) == rp->device)
    {
        switch(rp->code)
        {
            case IO_RP_ENUMERATE:
                if(NULL != ((struct PciDeviceData*)rp->device->privateData)->thisBridge)
                    status = PciEnumerate(rp->device->driverObject, rp->device, ((struct PciDeviceData*)rp->device->privateData)->thisBridge);
                else
                    goto DriverDispatchForwardRp;
                break;
            case IO_RP_GET_DEVICE_CONFIGURATION:
                if(IO_BUS_TYPE_PCI != rp->payload.deviceConfiguration.type)
                    goto DriverDispatchForwardRp;
                if(NULL != rp->device->privateData)
                {
                    struct PciDeviceData *info = rp->device->privateData;
                    status = PciReadConfigurationSpace(info->address, rp);
                    break;
                }
                status = IO_RP_PROCESSING_FAILED;
                break;
            case IO_RP_SET_DEVICE_CONFIGURATION:
                if(IO_BUS_TYPE_PCI != rp->payload.deviceConfiguration.type)
                    goto DriverDispatchForwardRp;
                if(NULL != rp->device->privateData)
                {
                    struct PciDeviceData *info = rp->device->privateData;
                    status = PciWriteConfigurationSpace(info->address, rp);
                    break;
                }
                status = IO_RP_PROCESSING_FAILED;
                break;
            case IO_RP_GET_BUS_CONFIGURATION:
                if(NULL != rp->device->privateData)
                {
                    struct PciDeviceData *info = rp->device->privateData;
                    rp->payload.busConfiguration.id = info->address;
                    rp->payload.busConfiguration.irqMap = info->irqMap;
                    rp->payload.busConfiguration.irq = info->irq;
                    status = OK;
                    break;
                }
                status = IO_RP_PROCESSING_FAILED;
                break;
            default:
                goto DriverDispatchForwardRp;
                break;
        }
    }
    else
        goto DriverDispatchForwardRp;
    rp->status = status;
    IoFinalizeRp(rp);
    return status;
DriverDispatchForwardRp:
    return IoSendRpDown(rp);
}

static STATUS DriverInit(struct ExDriverObject *driverObject)
{
    return OK;
} 

static STATUS DriverAddDevice(struct ExDriverObject *driverObject, struct IoSubDeviceObject *baseDeviceObject)
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

