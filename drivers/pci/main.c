#include "kernel.h"
#include "logging.h"
#include "device.h"
#include "utils.h"
#include "bridge.h"

static char driverName[] = "PCI host bridge driver";
static char driverVendor[] = "Standard drivers";
static char driverVersion[] = "1.0.0";

static void DriverProcessRp(struct IoDriverRp *rp)
{
    switch(rp->code)
    {
        default:
            break;
    }
    rp->status = NOT_IMPLEMENTED;
    IoFinalizeRp(rp);
}

static STATUS DriverDispatch(struct IoDriverRp *rp)
{
    STATUS status;
    switch(rp->code)
    {
        case IO_RP_ENUMERATE:
            if(NULL != ((struct PciDeviceData*)rp->device->privateData)->thisBridge)
                status = PciEnumerate(rp->device->driverObject, rp->device, ((struct PciDeviceData*)rp->device->privateData)->thisBridge);
            else
                status = IO_RP_PROCESSING_FAILED;
            break;
        default:
            status = NOT_IMPLEMENTED;
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

static STATUS DriverAddDevice(struct ExDriverObject *driverObject, struct IoSubDeviceObject *baseDeviceObject)
{
    struct IoSubDeviceObject *device;
    STATUS status;
    if(OK != (status = IoCreateSubDevice(driverObject, 0, &device)))
        return status;
    
    IoAttachSubDevice(device, baseDeviceObject);
    
    if(NULL == (device->privateData = MmAllocateKernelHeap(sizeof(struct PciDeviceData))))
        return OUT_OF_RESOURCES;
    
    struct PciDeviceData *deviceInfo = device->privateData;
    deviceInfo->enumerator = baseDeviceObject;
    deviceInfo->thisBridge = NULL;
    
    struct IoDriverRp *rp;
    if(OK != (status = IoCreateRp(&rp)))
        return status;
    
    rp->code = IO_RP_GET_BUS_CONFIGURATION;
    rp->flags = IO_DRIVER_RP_FLAG_SYNCHRONOUS;
    rp->payload.busConfiguration.device = baseDeviceObject;
    if(OK != (status = IoSendRp(baseDeviceObject, NULL, rp)))
    {
        LOG(SYSLOG_ERROR, "CHJJ %d", status);
        MmFreeKernelHeap(rp);
        return status;
    }

    struct PciAddress address;
    address.bus = rp->payload.busConfiguration.pci.bus;
    address.device = rp->payload.busConfiguration.pci.device;
    address.function = rp->payload.busConfiguration.pci.function;

    if(PciIsHostBridge(address))
    {
        struct PciBridge *b;
        if(PciIsMultifunction(address))
        {
            //multiple host bridges
            //multiple host bridge architecture is not supported for now, use only the first host bridge
            PciRegisterHostBridge(address, &b);
            deviceInfo->thisBridge = b;
        }
        else
        {
            //single host bridge
            PciRegisterHostBridge(address, &b);
            deviceInfo->thisBridge = b;
        }
        device->mainDeviceObject->type = IO_DEVICE_TYPE_BUS;
        LOG(SYSLOG_INFO, "PCI Host Bridge at %d:%d:%d\n", address.bus, address.device, address.function);
    }
    else if(PciIsPciPciBridge(address))
    {
        struct PciBridge *b;
        PciRegisterBridge(address, NULL, &b);
        deviceInfo->thisBridge = b;
        device->mainDeviceObject->type = IO_DEVICE_TYPE_BUS;
    }
    else
    {
        device->mainDeviceObject->type = IO_DEVICE_TYPE_NONE;
    }
    
    MmFreeKernelHeap(rp);
        

    return OK;
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

