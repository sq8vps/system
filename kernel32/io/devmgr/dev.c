#include "dev.h"
#include "mm/heap.h"
#include "common.h"
#include "assert.h"

//root device (ACPI or MP) object
static struct IoDeviceObject rootDevice;

STATUS IoCreateSubDevice(
    struct ExDriverObject *driver, 
    uint32_t privateSize, 
    enum IoDeviceType type, 
    IoDeviceFlags flags, 
    struct IoDeviceSubObject **device
    )
{
    ASSERT(driver);
    *device = MmAllocateKernelHeap(sizeof(struct IoDeviceSubObject));
    if(NULL == *device)
        return OUT_OF_RESOURCES;

    CmMemset(*device, 0, sizeof(struct IoDeviceSubObject));
    
    (*device)->driverObject = driver;
    (*device)->flags |= flags;
    (*device)->type = type;

    if(0 != privateSize)
        (*device)->privateData = MmAllocateKernelHeap(privateSize);
    
    //update list of created devices
    (*device)->nextDevice = driver->deviceObject;
    driver->deviceObject = *device;

    return OK;
}

struct IoDeviceSubObject* IoAttachSubDevice(struct IoDeviceSubObject *attachee, struct IoDeviceSubObject *destination)
{
    ASSERT(attachee && destination);
    while(destination->attachedDevice)
    {
        destination = destination->attachedDevice;
    }
    destination->attachedDevice = attachee;
    attachee->attachedTo = destination;
    attachee->mainDeviceObject = destination->mainDeviceObject;
    return destination;
}

STATUS IoRegisterDevice(struct IoDeviceSubObject *baseDevice, char *deviceId)
{
    ASSERT(baseDevice && deviceId);
    struct ExDriverObject **drivers = NULL;
    uint16_t driverCount = 0;
    STATUS ret = OK;

    struct IoDeviceObject *device = MmAllocateKernelHeap(sizeof(struct IoDeviceObject));
    if(NULL == device)
        return OUT_OF_RESOURCES;

    //find and load required drivers
    if(OK != (ret = ExLoadKernelDriversForDevice(deviceId, drivers, &driverCount)))
    {
        device->flags |= IO_DEVICE_FLAG_INITIALIZATION_FAILURE;
        goto exitIoRegisterDevice;
    }

    //store main device object in base subdevice object
    //new subdevice object will copy this field
    baseDevice->mainDeviceObject = device;
    
    //invoke AddDevice routine for all drivers to form a device stack
    for(uint16_t i = 0; i < driverCount; i++)
    {
        if(OK != (ret = drivers[i]->addDevice(drivers[i], baseDevice)))
        {
            device->flags |= IO_DEVICE_FLAG_INITIALIZATION_FAILURE;
            goto exitIoRegisterDevice;
        }
    }

    device->baseDevice = baseDevice;
    device->deviceId = MmAllocateKernelHeap(CmStrlen(deviceId));
    if(NULL == device->deviceId)
    {
        ret = OUT_OF_RESOURCES;
        device->flags |= IO_DEVICE_FLAG_INITIALIZATION_FAILURE;
        goto exitIoRegisterDevice;
    }
    CmStrcpy(device->deviceId, deviceId);

    device->parent = baseDevice->mainDeviceObject;
    if(NULL != baseDevice->mainDeviceObject->child)
    {
        baseDevice->mainDeviceObject->child->previous = device;
        device->next = baseDevice->mainDeviceObject->child->next;
    }
    baseDevice->mainDeviceObject->child = device;

    device->flags |= IO_DEVICE_FLAG_INITIALIZED;

exitIoRegisterDevice:
    MmFreeKernelHeap(drivers);
    return ret;
}

STATUS IoInitDeviceManager(char *rootDeviceId)
{
    ASSERT(rootDeviceId);
    STATUS ret = OK;
    struct ExDriverObject **drivers = NULL;
    uint16_t driverCount = 0;
    if(OK != (ret = ExLoadKernelDriversForDevice(rootDeviceId, drivers, &driverCount)))
        return ret;
    
    if(1 != driverCount)
    {
        MmFreeKernelHeap(drivers);
        return IO_ROOT_DEVICE_INIT_FAILURE;
    }

    struct IoDeviceSubObject *rootSubDevice = NULL;
    if(OK != (ret = IoCreateSubDevice(drivers[0], 0, IO_DEVICE_TYPE_ROOT, 0, &rootSubDevice)))
    {
        MmFreeKernelHeap(drivers);
        return ret;
    }

    rootDevice.flags = IO_DEVICE_FLAG_PERSISTENT;
    rootDevice.baseDevice = rootSubDevice;
    rootDevice.parent = NULL;

    rootDevice.deviceId = MmAllocateKernelHeap(CmStrlen(rootDeviceId));
    if(NULL == rootDevice.deviceId)
    {
        MmFreeKernelHeap(drivers);
        MmFreeKernelHeap(rootSubDevice);
        return IO_ROOT_DEVICE_INIT_FAILURE;
    }
    CmStrcpy(rootDevice.deviceId, rootDeviceId);

    rootDevice.flags |= IO_DEVICE_FLAG_INITIALIZED;

    return OK;
}

STATUS IoSetDeviceDisplayedName(struct IoDeviceSubObject *device, char *name)
{
    return NOT_IMPLEMENTED;
}