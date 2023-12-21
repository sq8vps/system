#include "device.h"
#include "logging.h"
#include "utils.h"
#include "bridge.h"
#include "class.h"

#define PCI_DEVICE_ID_PREFIX "PCI"

struct BusSubDeviceInfo
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
//     struct BusSubDeviceInfo *private;
//     if(NULL == (private = AcpiOsAllocate(sizeof(*private))))
//     {
//         ACPI_FREE(info);
//         AcpiOsFree(name.Pointer);
//         return AE_OK;
//     }
//     //create subdevice for new device
//     struct IoSubDeviceObject *dev;
//     if(OK != IoCreateSubDevice((struct ExDriverObject*)Context, IO_DEVICE_TYPE_NONE, 0, &dev))
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

static void addCompatibleDeviceId(struct IoDeviceObject *dev, char *class, char *subclass)
{
    char *deviceId;
    if(NULL != (deviceId = ExMakeDeviceId(3, PCI_DEVICE_ID_PREFIX, class, subclass)))
    {
        IoUpdateCompatibleDeviceIdList(dev, deviceId);
        MmFreeKernelHeap(deviceId);
    }
}

static bool alreadyEnumerated = false;

static STATUS PciEnumerateDeviceByAddress(union IoBusId address, struct ExDriverObject *drv, struct IoSubDeviceObject *dev)
{
    STATUS status;
    uint16_t vid, did;
    vid = PciGetVendorId(address);
    did = PciGetDeviceId(address);
    if(VENDOR_VALID(vid) && DEVICE_VALID(did))
    {
        //create subdevice for new device
        struct IoSubDeviceObject *newDev;
        if(OK != (status = IoCreateSubDevice(drv, 0, &newDev)))
        {
            return status;
        }

        //register new device in OS
        char vidS[5], didS[5];
        PciVenDevToString(vid, vidS);
        vidS[4] = '\0';
        PciVenDevToString(did, didS);
        didS[4] = '\0';
        char *deviceId;
        if(NULL == (deviceId = ExMakeDeviceId(3, PCI_DEVICE_ID_PREFIX, vidS, didS)))
            return status;
        
        if(OK != (status = IoRegisterDevice(newDev, deviceId)))
        {
            MmFreeKernelHeap(deviceId);
            return status;
        }

        if(NULL == (newDev->privateData = MmAllocateKernelHeap(sizeof(struct PciDeviceData))))
            return OUT_OF_RESOURCES;
        
        enum PciClass class = PciGetClass(address);
        enum PciSubclass subclass = PciGetSubclass(address);

        bool isPciBridge = false;

        switch(class)
        {
            case STORAGE:
                newDev->mainDeviceObject->type = IO_DEVICE_TYPE_STORAGE;
                switch(subclass)
                {
                    case STORAGE_IDE:
                        addCompatibleDeviceId(newDev->mainDeviceObject, "STORAGE", "IDE");
                        break;
                    case STORAGE_SATA:
                        addCompatibleDeviceId(newDev->mainDeviceObject, "STORAGE", "SATA");
                        break;
                    default:
                        break;
                }
                break;
            case BRIDGE:
                newDev->mainDeviceObject->type = IO_DEVICE_TYPE_BUS;
                switch(subclass)
                {
                    case BRIDGE_PCI:
                    case BRIDGE_PCI_2:
                        addCompatibleDeviceId(newDev->mainDeviceObject, "BUS", "PCI");
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
                newDev->mainDeviceObject->type = IO_DEVICE_TYPE_OTHER;
                break;
        }

        char *name = PciGetGenericDeviceName(class, subclass);
        if(NULL != name)
            IoSetDeviceDisplayedName(newDev, name);

        LOG(SYSLOG_INFO, "Device %s (%s) found at %u:%u:%u", deviceId, name ? name : "", address.pci.bus, address.pci.device, address.pci.function);
    
        struct PciDeviceData *deviceInfo = newDev->privateData;
        deviceInfo->address = address;
        deviceInfo->thisBridge = NULL;

        if(isPciBridge) 
        {
            deviceInfo->irqMap = ((struct PciDeviceData*)dev->privateData)->irqMap->child;
            while(NULL != deviceInfo->irqMap)
            {
                if(deviceInfo->irqMap->id.pci.device == address.pci.device)
                {
                    break;
                }
                deviceInfo->irqMap = deviceInfo->irqMap->next;
            }
        }
        else
        {
            deviceInfo->irqMap = NULL;
            uint32_t k = 0;
            for(uint32_t i = 0; i < ((struct PciDeviceData*)dev->privateData)->irqMap->irqCount; i++)
            {
                if(((struct PciDeviceData*)dev->privateData)->irqMap->irq[i].id.pci.device == address.pci.device)
                {
                    deviceInfo->irqEntry[k++] = ((struct PciDeviceData*)dev->privateData)->irqMap->irq[i];
                    LOG(SYSLOG_INFO, "IRQ at pin %lu, GSI %lu", deviceInfo->irqEntry[k - 1].pin, deviceInfo->irqEntry[k - 1].gsi);
                    if(PCI_DEVICE_IRQ_LINE_COUNT == k)
                        break;
                }
            }
        }

        MmFreeKernelHeap(deviceId);

        IoInitializeDevice(newDev->mainDeviceObject);
    }
    return OK;
}

STATUS PciEnumerate(struct ExDriverObject *drv, struct IoSubDeviceObject *dev, struct PciBridge *bridge)
{
    if(!((dev->mainDeviceObject->type == IO_DEVICE_TYPE_BUS) || (dev->mainDeviceObject->flags & IO_DEVICE_FLAG_ENUMERATION_CAPABLE)))
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

STATUS PciAddDevice(struct ExDriverObject *driverObject, struct IoSubDeviceObject *baseDeviceObject)
{
    struct IoSubDeviceObject *device;
    STATUS status;
    if(OK != (status = IoCreateSubDevice(driverObject, 0, &device)))
        return status;
    
    IoAttachSubDevice(device, baseDeviceObject);
    
    if(NULL == (device->privateData = MmAllocateKernelHeap(sizeof(struct PciDeviceData))))
        return OUT_OF_RESOURCES;
    
    struct PciDeviceData *deviceInfo = device->privateData;
    deviceInfo->thisBridge = NULL;

    struct IoDriverRp *rp;
    if(OK != (status = IoCreateRp(&rp)))
        return status;
    
    rp->code = IO_RP_GET_BUS_CONFIGURATION;
    rp->flags = IO_DRIVER_RP_FLAG_SYNCHRONOUS;
    rp->payload.busConfiguration.device = baseDeviceObject;
    rp->payload.busConfiguration.type = IO_BUS_TYPE_PCI;
    if(OK != (status = IoSendRp(baseDeviceObject, NULL, rp)))
    {
        MmFreeKernelHeap(rp);
        return status;
    }

    deviceInfo->address = rp->payload.busConfiguration.id;
    deviceInfo->irqMap = rp->payload.busConfiguration.irq;

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
        device->mainDeviceObject->type = IO_DEVICE_TYPE_BUS;
        LOG(SYSLOG_INFO, "PCI Host Bridge at %d:%d:%d\n", deviceInfo->address.pci.bus, deviceInfo->address.pci.device, deviceInfo->address.pci.function);
    }
    else if(PciIsPciPciBridge(deviceInfo->address))
    {
        struct PciBridge *b;
        PciRegisterBridge(deviceInfo->address, NULL, &b);
        deviceInfo->thisBridge = b;
        device->mainDeviceObject->type = IO_DEVICE_TYPE_BUS;
    }
    else
    {
        device->mainDeviceObject->type = IO_DEVICE_TYPE_BUS;
    }
    
    MmFreeKernelHeap(rp);
        
    return OK;
}