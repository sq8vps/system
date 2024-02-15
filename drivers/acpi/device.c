#include "device.h"
#include "kernel.h"
#include "logging.h"

static bool alreadyEnumerated = false;
static bool pciHostBridgeFound = false; //allow only one PCI/PCI-E host bridge

#define ACPI_DEVICE_ID_PREFIX "ACPI"

ACPI_STATUS DriverGetBusInfoForDevice(struct IoDriverRp *rp)
{
    if(NULL != rp->device)
    {
        struct BusSubDeviceInfo *busConfig = ((struct BusSubDeviceInfo*)(rp->device->privateData));
        if(NULL != busConfig)
        {
            if(IO_BUS_TYPE_PCI == busConfig->type)
            {
                rp->payload.busConfiguration.id.pci.bus = busConfig->id.pci.bus;
                rp->payload.busConfiguration.id.pci.device = busConfig->id.pci.device;
                rp->payload.busConfiguration.id.pci.function = busConfig->id.pci.function;
            }
            rp->payload.busConfiguration.irqMap = busConfig->irqMap;
            return AE_OK;
        }
    }
    return AE_NOT_FOUND;
}

static ACPI_STATUS DriverEnumerationCallback(ACPI_HANDLE Object, UINT32 NestingLevel, void *Context, void **ReturnValue)
{
    if(NestingLevel > 2)
        return AE_OK;

    ACPI_DEVICE_INFO *info;
    ACPI_OBJECT_TYPE type;
    if((AE_OK != AcpiGetType(Object, &type)) || (type != ACPI_TYPE_DEVICE))
        return AE_OK;
    if((AE_OK != AcpiGetObjectInfo(Object, &info)) || (NULL == info))
        return AE_OK;
    //find only devices that provide some Hardware ID
    if((0 == info->HardwareId.Length) || (!(info->Valid & ACPI_VALID_HID)))
    {
        ACPI_FREE(info);
        return AE_OK;
    }
    //exclude objects not being actual devices
    if(!CmStrcmp(info->HardwareId.String, "PNP0A05") //container
    || !CmStrcmp(info->HardwareId.String, "PNP0A06") //container
    || !CmStrcmp(info->HardwareId.String, "PNP0C0F") //PCI interrupt link device
    )
    {
        ACPI_FREE(info);
        return AE_OK;
    }

    //check if PCI/PCI-E host bridge was enumerated previously, i.e., allow only one host bridge
    //PNP0A03 is the PCI host bridge, PNP0A08 is the PCI-E host bridge
    bool isPciHostBridge = !CmStrcmp(info->HardwareId.String, "PNP0A03") || !CmStrcmp(info->HardwareId.String, "PNP0A08");
    if(pciHostBridgeFound && isPciHostBridge)
    {
        ACPI_FREE(info);
        return AE_OK;
    }

    //get device ACPI name/path
    ACPI_BUFFER name;
    name.Length = ACPI_ALLOCATE_BUFFER;
    name.Pointer = NULL;
    if(ACPI_FAILURE(AcpiGetName(Object, ACPI_FULL_PATHNAME, &name)))
    {
        ACPI_FREE(info);
        return AE_OK;
    }
    //prepare space for private data for the bus subdevice
    struct BusSubDeviceInfo *private;
    if(NULL == (private = AcpiOsAllocateZeroed(sizeof(*private))))
    {
        ACPI_FREE(info);
        AcpiOsFree(name.Pointer);
        return AE_OK;
    }
    //create subdevice for new device
    struct IoSubDeviceObject *dev;
    if(OK != IoCreateSubDevice((struct ExDriverObject*)Context, 0, &dev))
    {
        ACPI_FREE(info);
        AcpiOsFree(name.Pointer);
        AcpiOsFree(private);
        return AE_OK;
    }

    //store ACPI path for the new device
    private->path = name.Pointer;
    //store PCI bus number
    if(isPciHostBridge)
    {
        pciHostBridgeFound = true;
        private->type = IO_BUS_TYPE_PCI;
        private->id.pci.bus = 0; //PCI bus = 0 for host controller
        private->id.pci.device = PCI_ADR_EXTRACT_DEVICE(info->Address);
        private->id.pci.function = PCI_ADR_EXTRACT_FUNCTION(info->Address);
    }
    else
    {
        private->type = IO_BUS_TYPE_ACPI;
    }
    dev->privateData = private;
    //register new device in OS
    char *deviceId = ExMakeDeviceId(2, ACPI_DEVICE_ID_PREFIX, info->HardwareId.String);
    if(OK != IoRegisterDevice(dev, deviceId))
    {
        MmFreeKernelHeap(deviceId);
        ACPI_FREE(info);
        AcpiOsFree(private);
        return AE_OK;
    }
    if(isPciHostBridge)
    {
        char *t = ExMakeDeviceId(2, ACPI_DEVICE_ID_PREFIX, "PCI");
        if(t != NULL)
        {
            IoUpdateCompatibleDeviceIdList(dev->mainDeviceObject, t);
            MmFreeKernelHeap(t);
        }

        AcpiGetPciIrqTree(Object, &(private->irqMap));
    }
    else
    {
        AcpiGetIrq(Object, private);
    }
    char *friendlyName = AcpiGetPnpName(info->HardwareId.String);
    if(NULL != friendlyName)
        IoSetDeviceDisplayedName(dev, friendlyName);
    
    IoWriteSyslog(AcpiLogHandle, SYSLOG_INFO, "Device found at %s, device ID is %s\n", (char*)name.Pointer, deviceId);
    IoInitializeDevice(dev->mainDeviceObject);

    MmFreeKernelHeap(deviceId);
    ACPI_FREE(info);
    return AE_OK;
}

ACPI_STATUS DriverEnumerate(struct ExDriverObject *drv)
{
    void *ret;
    if(alreadyEnumerated)
        return AE_OK;
    alreadyEnumerated = true;
    return AcpiGetDevices(NULL, DriverEnumerationCallback, drv, &ret);
}

