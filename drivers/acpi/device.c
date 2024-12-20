#include "device.h"
#include "kernel.h"
#include "logging.h"

#include "acclib.h"

static bool alreadyEnumerated = false;
static bool pciHostBridgeFound = false; //allow only one PCI/PCI-E host bridge

#define ACPI_DEVICE_ID_PREFIX "ACPI"

STATUS AcpiGetDeviceLocation(struct IoRp *rp)
{
    struct IoDeviceObject *dev = IoGetCurrentRpPosition(rp);

    struct AcpiDeviceInfo *busConfig = ((struct AcpiDeviceInfo*)(dev->privateData));
    if(NULL != busConfig)
    {
        if(IO_BUS_TYPE_PCI == busConfig->type)
        {
            rp->payload.location.id.pci.bus = busConfig->id.pci.bus;
            rp->payload.location.id.pci.device = busConfig->id.pci.device;
            rp->payload.location.id.pci.function = busConfig->id.pci.function;
            rp->payload.location.type = IO_BUS_TYPE_PCI;
        }
        return OK;
    }
    return RP_PROCESSING_FAILED;
}

STATUS AcpiGetDeviceResources(struct IoRp *rp)
{
    struct IoDeviceObject *dev = IoGetCurrentRpPosition(rp);

    struct AcpiDeviceInfo *devInfo = ((struct AcpiDeviceInfo*)(dev->privateData));
    if(NULL != devInfo)
    {
        rp->payload.resource.count = 0;
        if(0 == devInfo->resourceCount)
        {
            return OK;
        }
        else
        {
            uintptr_t size = devInfo->resourceCount * sizeof(devInfo->resource[0]);
            rp->payload.resource.res = MmAllocateKernelHeap(size);
            if(NULL == rp->payload.resource.res)
                return OUT_OF_RESOURCES;
            RtlMemcpy(rp->payload.resource.res, devInfo->resource, size);
            rp->payload.resource.count = devInfo->resourceCount;
            return OK;
        }
    }
    return RP_PROCESSING_FAILED;
}

struct AcpiEnumerationContext
{
    struct ExDriverObject *driver;
    struct IoDeviceObject *dev;
};

static ACPI_STATUS AcpiEnumerationCallback(ACPI_HANDLE Object, UINT32 NestingLevel, void *Context, void **ReturnValue)
{
    // if(NestingLevel > 2)
    //     return AE_OK;

    struct AcpiEnumerationContext *ctx = Context;

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
    if(!RtlStrcmp(info->HardwareId.String, "PNP0A05") //container
    || !RtlStrcmp(info->HardwareId.String, "PNP0A06") //container
    || !RtlStrcmp(info->HardwareId.String, "PNP0C0F") //PCI interrupt link device
    )
    {
        ACPI_FREE(info);
        return AE_OK;
    }

    //check if PCI/PCI-E host bridge was enumerated previously, i.e., allow only one host bridge
    //PNP0A03 is the PCI host bridge, PNP0A08 is the PCI-E host bridge
    bool isPciHostBridge = ACPI_IS_PCI_HOST_BRIDGE(info->HardwareId.String);
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
    struct AcpiDeviceInfo *private;
    if(NULL == (private = AcpiOsAllocateZeroed(sizeof(*private) + name.Length + 1)))
    {
        ACPI_FREE(info);
        AcpiOsFree(name.Pointer);
        return AE_OK;
    }

    RtlStrncpy(private->pnpId, info->HardwareId.String, ACPI_PNP_ID_MAX_LENGTH);
    RtlStrcpy(private->path, name.Pointer);

    AcpiOsFree(name.Pointer);

    //create device for new device
    struct IoDeviceObject *dev;
    if(OK != IoCreateDevice(ctx->driver, IO_DEVICE_TYPE_OTHER, 0, &dev))
    {
        ACPI_FREE(info);
        AcpiOsFree(private);
        return AE_OK;
    }

    //store PCI bus number
    if(isPciHostBridge)
    {
        pciHostBridgeFound = true;
        private->type = IO_BUS_TYPE_PCI;
        private->id.pci.bus = 0; //PCI bus = 0 for host controller
        private->id.pci.device = PCI_ADR_EXTRACT_DEVICE(info->Address);
        private->id.pci.function = PCI_ADR_EXTRACT_FUNCTION(info->Address);
        dev->type = IO_DEVICE_TYPE_BUS;
    }
    else
    {
        private->type = IO_BUS_TYPE_ACPI;
    }
    dev->privateData = private;

    ACPI_FREE(info);

    if(ACPI_FAILURE(AcpiFillResourceList(Object, private)))
    {
        if(NULL != private->resource)
            MmFreeKernelHeap(private->resource);
        AcpiOsFree(private);
        IoDestroyDevice(dev);
        return AE_OK;
    }

    //register new device in OS
    if(OK != IoRegisterDevice(dev, ctx->dev))
    {
        if(NULL != private->resource)
            MmFreeKernelHeap(private->resource);
        AcpiOsFree(private);
        IoDestroyDevice(dev);
        return AE_OK;
    }

    IoWriteSyslog(AcpiLogHandle, SYSLOG_INFO, "Device found at %s (HID: %s)", private->path, private->pnpId);

    return AE_OK;
}

ACPI_STATUS DriverEnumerate(struct ExDriverObject *drv, struct IoDeviceObject *dev)
{
    struct AcpiEnumerationContext ctx;
    ctx.driver = drv;
    ctx.dev = dev;
    void *ret;
    if(alreadyEnumerated)
        return AE_OK;
    alreadyEnumerated = true;
    return AcpiGetDevices(NULL, AcpiEnumerationCallback, &ctx, &ret);
}

STATUS AcpiGetDeviceId(struct IoRp *rp)
{
    if(NULL != rp->device->privateData)
    {
        struct AcpiDeviceInfo *info = IoGetCurrentRpPosition(rp)->privateData;

        char *deviceId, **compatibleIds;
        uint8_t compatibleIdCount = 0;

        switch(info->type)
        {
            case IO_BUS_TYPE_PCI:
                compatibleIdCount = 1;
                break;
            default:
                break;
        }

        deviceId = MmAllocateKernelHeap(128);
        if(NULL == deviceId)
        {
            rp->status = OUT_OF_RESOURCES;
            return OUT_OF_RESOURCES;
        }

        compatibleIds = RtlAllocateStringTable(IO_MAX_COMPATIBLE_DEVICE_IDS, compatibleIdCount, 128);
        if(NULL == compatibleIds)
        {
            MmFreeKernelHeap(deviceId);
            rp->status = OUT_OF_RESOURCES;
            return OUT_OF_RESOURCES;
        }

        snprintf(deviceId, 128, ACPI_DEVICE_ID_PREFIX "/%s", info->pnpId);
        
        if(IO_BUS_TYPE_PCI == info->type)
        {
            snprintf(compatibleIds[0], 128, "BUS/PCI");
        }

        rp->payload.deviceId.mainId = deviceId;
        rp->payload.deviceId.compatibleId = compatibleIds;
        return OK;
    }
    return RP_PROCESSING_FAILED;
}