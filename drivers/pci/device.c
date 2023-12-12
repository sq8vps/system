#include "device.h"
#include "logging.h"
#include "utils.h"
#include "bridge.h"

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

static bool alreadyEnumerated = false;

static STATUS PciEnumerateDeviceByAddress(struct PciAddress address, struct ExDriverObject *drv, struct IoSubDeviceObject *dev)
{
    STATUS status;
    uint16_t vid, did;
    vid = PciGetVendorId(address);
    did = PciGetDeviceId(address);
    if(VENDOR_VALID(vid) && DEVICE_VALID(did))
    {
        //create subdevice for new device
        struct IoSubDeviceObject *newDev;
        if(OK != (status = IoCreateSubDevice(dev->driverObject, 0, &newDev)))
        {
            return status;
        }

        //register new device in OS
        char vidS[5], didS[5];
        PciVenDevToString(vid, vidS);
        vidS[4] = '\0';
        PciVenDevToString(did, didS);
        didS[4] = '\0';
        char *deviceId = ExMakeDeviceId(3, PCI_DEVICE_ID_PREFIX, vidS, didS);
        if(OK != (status = IoRegisterDevice(newDev, deviceId)))
        {
            return status;
        }

        if(OK != (status = IoBuildDeviceStack(newDev->mainDeviceObject)))
        {
            return status;
        }
        
        LOG(SYSLOG_INFO, "Device %s found\n", deviceId);
    }
    return OK;
}

STATUS PciEnumerate(struct ExDriverObject *drv, struct IoSubDeviceObject *dev, struct PciBridge *bridge)
{
    STATUS status;
    if(alreadyEnumerated)
        return OK;
    alreadyEnumerated = true;
    
    for(uint16_t bus = bridge->secondaryBus; bus < (bridge->subordinateBus + 1); bus++)
    {
        for(uint16_t device = 0; device < 32; device++)
        {
            struct PciAddress a = {.bus = bus, .device = device, .function = 0};
            if(PciIsMultifunction(a))
            {
                for(uint16_t function = 0; function < 8; function++)
                {
                    a.function = function;
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