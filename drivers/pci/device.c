#include "device.h"
#include "logging.h"
#include "utils.h"
#include "bridge.h"
#include "class.h"
#include "io/dev/dev.h"

#define PCI_DEVICE_ID_PREFIX "PCI"

struct AcpiDeviceInfo
{
    char *path;
};

static void PciVenDevToString(uint16_t n, char *s)
{
    for(uint8_t i = 0; i < 4; i++)
    {
        uint8_t k = (n >> (i * 4)) & 0xF;
        if(k < 10)
            s[3 - i] = k + '0';
        else
            s[3 - i] = k - 10 + 'A'; 
    }
}

// #define ACPI_DEVICE_ID_PREFIX "ACPI"
// static char* makeDeviceId(char *hid)
// {
//     char *t = MmAllocateKernelHeap(CmStrlen(ACPI_DEVICE_ID_PREFIX) + 1 + CmStrlen(hid) + 1);
//     if(NULL == t)
//         return NULL;
//     CmStrcpy(t, ACPI_DEVICE_ID_PREFIX "/");
//     CmStrcat(t, hid);
//     return t;
// }


//     //prepare space for private data for the bus subdevice
//     struct AcpiDeviceInfo *private;
//     if(NULL == (private = AcpiOsAllocate(sizeof(*private))))
//     {
//         ACPI_FREE(info);
//         AcpiOsFree(name.Pointer);
//         return AE_OK;
//     }
//     //create subdevice for new device
//     struct IoDeviceObject *dev;
//     if(OK != IoCreateDevice((struct ExDriverObject*)Context, IO_DEVICE_TYPE_NONE, 0, &dev))
//     {
//         ACPI_FREE(info);
//         AcpiOsFree(name.Pointer);
//         AcpiOsFree(private);
//         return AE_OK;
//     }
//     //store ACPI path for the new device
//     private->path = name.Pointer;
//     dev->privateData = private;
//     //register new device in OS
//     char *deviceId = makeDeviceId(info->HardwareId.String);
//     IoRegisterDevice(dev, deviceId);
    
//     IoWriteSyslog(AcpiLogHandle, SYSLOG_INFO, "Device found at %s, enumerating as %s\n", (char*)name.Pointer, deviceId);

static bool alreadyEnumerated = false;

static STATUS PciEnumerateDeviceByAddress(union IoBusId address, struct ExDriverObject *drv, struct IoDeviceObject *enumerator)
{
    STATUS status;
    uint16_t vid, did;
    vid = PciGetVendorId(address);
    did = PciGetDeviceId(address);
    if(VENDOR_VALID(vid) && DEVICE_VALID(did))
    {
        enum PciClass class = PciGetClass(address);
        enum PciSubclass subclass = PciGetSubclass(address);

        bool isPciBridge = false;
        enum IoDeviceType type;

        switch(class)
        {
            case STORAGE:
                type = IO_DEVICE_TYPE_STORAGE;
                break;
            case BRIDGE:
                type = IO_DEVICE_TYPE_BUS;
                switch(subclass)
                {
                    case BRIDGE_PCI:
                    case BRIDGE_PCI_2:
                        isPciBridge = true;
                        break;
                    default:
                        break;
                }
                break;
            case NETWORK:
            case DISPLAY:
            case MULTIMEDIA:
            case MEMORY:
            case SIMPLE_COMM:
            case SYSTEM_PERIPH:
            case INPUT_DEVICE:
            case DOCKING_STATION:
            case PROCESSOR:
            case SERIAL_BUS:
            case WIRELESS:
            case INTELLIGENT:
            case SATELLITE_COMM:
            case ENCRYPTION:
            case SIGNAL_PROCESSING:
            default:
                type = IO_DEVICE_TYPE_OTHER;
                break;
        }

        //create subdevice for new device
        struct IoDeviceObject *dev;
        if(OK != (status = IoCreateDevice(drv, type, 0, &dev)))
        {
            return status;
        }


        if(NULL == (dev->privateData = MmAllocateKernelHeapZeroed(sizeof(struct PciDeviceData))))
            return OUT_OF_RESOURCES;


        LOG(SYSLOG_INFO, "Device %X/%X found at %u:%u:%u", vid, did, address.pci.bus, address.pci.device, address.pci.function);
    
        struct PciDeviceData *deviceInfo = dev->privateData;
        deviceInfo->address = address;
        deviceInfo->thisBridge = NULL;
        deviceInfo->class = class;
        deviceInfo->subclass = subclass;
        deviceInfo->vendor = vid;
        deviceInfo->device = did;

        struct PciDeviceData *enumeratorInfo = enumerator->privateData;

        if(isPciBridge) 
        {
            //if this is a bridge, find the part of IRQ map that belongs to this bridge
            //TODO: untested!!!
            struct IoIrqMap *r = deviceInfo->irqMap;
            while(NULL != r)
            {
                if(address.pci.device == r->id.pci.device)
                    break;
                r = r->next;
            }

            if(NULL != r)
            {
                deviceInfo->irqMap = IoCopyIrqMap(r);
            }
        }
        else
        {
            deviceInfo->irqMap = NULL;
            for(uint32_t i = 0; i < enumeratorInfo->irqMap->irqCount; i++)
            {
                if(enumeratorInfo->irqMap->irq[i].id.pci.device == address.pci.device)
                {
                    if(PciGetInterruptPin(address) == enumeratorInfo->irqMap->irq[i].pin)
                    {
                        deviceInfo->irqAvailable = 1;
                        deviceInfo->irq = enumeratorInfo->irqMap->irq[i];
                        LOG(SYSLOG_INFO, "IRQ at pin %lu, GSI %lu", deviceInfo->irq.pin, deviceInfo->irq.gsi);
                        break;
                    }
                }
            }
        }

        if(OK != (status = IoRegisterDevice(dev, enumerator)))
        {
            return status;
        }

    }
    return OK;
}

STATUS PciEnumerate(struct ExDriverObject *drv, struct IoDeviceObject *dev, struct PciBridge *bridge)
{
    if(!((dev->type == IO_DEVICE_TYPE_BUS) || (dev->flags & IO_DEVICE_FLAG_ENUMERATION_CAPABLE)))
        return OPERATION_NOT_ALLOWED;
    if(alreadyEnumerated)
        return OK;
    alreadyEnumerated = true;
    
    for(uint16_t bus = bridge->secondaryBus; bus < (bridge->subordinateBus + 1); bus++)
    {
        for(uint16_t device = 0; device < 32; device++)
        {
            union IoBusId a = {.pci.bus = bus, .pci.device = device, .pci.function = 0};
            if(PciIsMultifunction(a))
            {
                for(uint16_t function = 0; function < 8; function++)
                {
                    a.pci.function = function;
                    PciEnumerateDeviceByAddress(a, drv, dev);
                }
            }
            else
            {
                PciEnumerateDeviceByAddress(a, drv, dev);
            }
        }
    }
    return OK;
}

STATUS PciAddDevice(struct ExDriverObject *driverObject, struct IoDeviceObject *baseDeviceObject)
{
    struct IoDeviceObject *device;
    STATUS status;
    if(OK != (status = IoCreateDevice(driverObject, IO_DEVICE_TYPE_OTHER, 0, &device)))
        return status;
    
    //TODO: device destruction on failure

    if(NULL == (device->privateData = MmAllocateKernelHeapZeroed(sizeof(struct PciDeviceData))))
        return OUT_OF_RESOURCES;
    
    struct PciDeviceData *deviceInfo = device->privateData;
    deviceInfo->thisBridge = NULL;

    enum IoBusType busType;
    status = IoGetDeviceLocation(baseDeviceObject, &busType, &(deviceInfo->address));
    if(OK != status)
    {
        goto _PciAddDeviceFailure;
    }
    
    if(IO_BUS_TYPE_PCI != busType)
    {
        status = SYSTEM_INCOMPATIBLE;
        goto _PciAddDeviceFailure;
    }

    struct IoDeviceResource *resource;
    uint32_t resourceCount;
    status = IoGetDeviceResources(baseDeviceObject, &resource, &resourceCount);
    if(OK != status)
    {
        goto _PciAddDeviceFailure;
    }

    for(uint32_t i = 0; i < resourceCount; i++)
    {
        if(IO_RESOURCE_IRQ_MAP == resource[i].type)
        {
            deviceInfo->irqMap = IoCopyIrqMap(&(resource[i].irqMap));
            break;
        }
    }

    if(NULL != resource)
        MmFreeKernelHeap(resource);

    IoAttachDevice(device, baseDeviceObject);

    if(PciIsHostBridge(deviceInfo->address))
    {
        struct PciBridge *b;
        if(PciIsMultifunction(deviceInfo->address))
        {
            //multiple host bridges
            //multiple host bridge architecture is not supported for now, use only the first host bridge
            PciRegisterHostBridge(deviceInfo->address, &b);
            deviceInfo->thisBridge = b;
        }
        else
        {
            //single host bridge
            PciRegisterHostBridge(deviceInfo->address, &b);
            deviceInfo->thisBridge = b;
        }
        device->type = IO_DEVICE_TYPE_BUS;
        LOG(SYSLOG_INFO, "PCI Host Bridge at %d:%d:%d\n", deviceInfo->address.pci.bus, deviceInfo->address.pci.device, deviceInfo->address.pci.function);
    }
    else if(PciIsPciPciBridge(deviceInfo->address))
    {
        struct PciBridge *b;
        PciRegisterBridge(deviceInfo->address, NULL, &b);
        deviceInfo->thisBridge = b;
        device->type = IO_DEVICE_TYPE_BUS;
    }
    else
    {
        device->type = IO_DEVICE_TYPE_BUS;
    }
    
        
    return OK;
_PciAddDeviceFailure:
    MmFreeKernelHeap(device->privateData);
    return status;
}

STATUS PciGetSystemDeviceId(struct IoRp *rp)
{
    struct IoDeviceObject *dev = IoGetCurrentRpPosition(rp);

    if(NULL != dev->privateData)
    {
        struct PciDeviceData *info = dev->privateData;
        char *deviceId, **compatibleIds;

        char vidS[5], didS[5];
        PciVenDevToString(info->vendor, vidS);
        vidS[4] = '\0';
        PciVenDevToString(info->device, didS);
        didS[4] = '\0';

        deviceId = ExMakeDeviceId(3, PCI_DEVICE_ID_PREFIX, vidS, didS);

        if(NULL == deviceId)
        {
            rp->status = OUT_OF_RESOURCES;
            return OUT_OF_RESOURCES;
        }

        compatibleIds = MmAllocateKernelHeapZeroed(IO_MAX_COMPATIBLE_DEVICE_IDS * sizeof(*compatibleIds));
        if(NULL == compatibleIds)
        {
            MmFreeKernelHeap(deviceId);
            rp->status = OUT_OF_RESOURCES;
            return OUT_OF_RESOURCES;
        }
        switch(info->class)
        {
            case STORAGE:
                switch(info->subclass)
                {
                    case STORAGE_IDE:
                        compatibleIds[0] = ExMakeDeviceId(3, PCI_DEVICE_ID_PREFIX, "STORAGE", "IDE");
                        break;
                    case STORAGE_SATA:
                        compatibleIds[0] = ExMakeDeviceId(3, PCI_DEVICE_ID_PREFIX, "STORAGE", "AHCI");
                        break;
                    default:
                        break;
                }
                break;
            case BRIDGE:
                switch(info->subclass)
                {
                    case BRIDGE_PCI:
                    case BRIDGE_PCI_2:
                        compatibleIds[0] = ExMakeDeviceId(3, PCI_DEVICE_ID_PREFIX, "BUS", "PCI");
                        break;
                    default:
                        break;
                }
                break;
            case NETWORK:
            case DISPLAY:
            case MULTIMEDIA:
            case MEMORY:
            case SIMPLE_COMM:
            case SYSTEM_PERIPH:
            case INPUT_DEVICE:
            case DOCKING_STATION:
            case PROCESSOR:
            case SERIAL_BUS:
            case WIRELESS:
            case INTELLIGENT:
            case SATELLITE_COMM:
            case ENCRYPTION:
            case SIGNAL_PROCESSING:
            default:
                break;
        }
        


        rp->payload.deviceId.mainId = deviceId;
        rp->payload.deviceId.compatibleId = compatibleIds;
        return OK;
    }
    return IO_RP_PROCESSING_FAILED;
}

STATUS PciGetResources(struct IoRp *rp)
{
    struct IoDeviceObject *dev = IoGetCurrentRpPosition(rp);
    if(NULL != dev->privateData)
    {
        struct PciDeviceData *info = dev->privateData;
        
        rp->payload.resource.count = 0;
        if(NULL != info->irqMap)
        {
            rp->payload.resource.res = MmAllocateKernelHeap(sizeof(rp->payload.resource));
            if(NULL != rp->payload.resource.res)
            {
                rp->payload.resource.count = 1;
                rp->payload.resource.res->irqMap = *(info->irqMap);
                rp->payload.resource.res->type = IO_RESOURCE_IRQ_MAP;
                return OK;
            }
            
            return OUT_OF_RESOURCES;
        }
        else if(info->irqAvailable)
        {
            rp->payload.resource.res = MmAllocateKernelHeap(sizeof(rp->payload.resource));
            if(NULL != rp->payload.resource.res)
            {
                rp->payload.resource.count = 1;
                rp->payload.resource.res->irq = info->irq;
                rp->payload.resource.res->type = IO_RESOURCE_IRQ;
                return OK;
            }
            
            return OUT_OF_RESOURCES;
        }

        return OK;
    }
    return IO_RP_PROCESSING_FAILED;    
}