#include "dev.h"
#include "mm/heap.h"
#include "common.h"
#include "assert.h"
#include "enumeration.h"

//root device (ACPI or MP) object
static struct IoDeviceObject rootDevice = {.type = IO_DEVICE_TYPE_ROOT};

STATUS IoCreateSubDevice(
    struct ExDriverObject *driver, 
    IoDeviceFlags flags, 
    struct IoSubDeviceObject **device
    )
{
    ASSERT(driver);
    *device = MmAllocateKernelHeap(sizeof(**device));
    if(NULL == *device)
        return OUT_OF_RESOURCES;

    CmMemset(*device, 0, sizeof(**device));
    
    (*device)->driverObject = driver;
    (*device)->flags |= flags;
    
    //update list of created devices
    (*device)->nextDevice = driver->deviceObject;
    driver->deviceObject = *device;

    return OK;
}


struct IoSubDeviceObject* IoAttachSubDevice(struct IoSubDeviceObject *attachee, struct IoSubDeviceObject *destination)
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

STATUS IoRegisterDevice(struct IoSubDeviceObject *baseDevice, char *deviceId)
{
    ASSERT(baseDevice && deviceId);

    struct IoDeviceObject *device = MmAllocateKernelHeap(sizeof(*device));
    if(NULL == device)
        return OUT_OF_RESOURCES;
    
    CmMemset(device, 0, sizeof(*device));

    //store main device object in base subdevice object
    //new subdevice object will copy this field
    baseDevice->mainDeviceObject = device;

    device->type = IO_DEVICE_TYPE_NONE;
    device->baseDevice = baseDevice;
    device->deviceId = MmAllocateKernelHeap(CmStrlen(deviceId));
    if(NULL == device->deviceId)
    {
        device->flags |= IO_DEVICE_FLAG_INITIALIZATION_FAILURE;
        return OUT_OF_RESOURCES;
    }
    CmStrcpy(device->deviceId, deviceId);

    device->parent = baseDevice->mainDeviceObject;
    if(NULL != baseDevice->mainDeviceObject->child)
    {
        baseDevice->mainDeviceObject->child->previous = device;
        device->next = baseDevice->mainDeviceObject->child->next;
    }
    baseDevice->mainDeviceObject->child = device;


    return OK;
}

STATUS IoInitializeDevice(struct IoDeviceObject *dev)
{
    dev->flags |= IO_DEVICE_FLAG_INITIALIZED;
    return IoNotifyDeviceEnumerator(dev);
}

STATUS IoBuildDeviceStack(struct IoDeviceObject *device)
{
    struct ExDriverObjectList *drivers = NULL;
    uint16_t driverCount = 0;
    STATUS ret;
    //find and load required drivers
    if(OK != (ret = ExLoadKernelDriversForDevice(device->deviceId, &drivers, &driverCount)))
    {
        device->flags |= IO_DEVICE_FLAG_INITIALIZATION_FAILURE;
        MmFreeKernelHeap(drivers);
        return ret;
    }

    //invoke AddDevice routine for all drivers to form a device stack
    struct ExDriverObjectList *d = drivers;
    for(uint16_t i = 0; i < driverCount; i++)
    {
        if(OK != (ret = d->this->addDevice(d->this, device->baseDevice)))
        {
            device->flags |= IO_DEVICE_FLAG_INITIALIZATION_FAILURE;
            MmFreeKernelHeap(drivers);
            return ret;
        }
        d = d->next;
    }
    device->flags |= IO_DEVICE_FLAG_READY_TO_RUN;
    MmFreeKernelHeap(drivers);
    return OK;
}

STATUS IoInitDeviceManager(char *rootDeviceId)
{
    ASSERT(rootDeviceId);
    STATUS ret = OK;
    struct ExDriverObjectList *drivers = NULL;
    uint16_t driverCount = 0;
    
    if(OK != (ret = IoStartDeviceEnumerationThread()))
        return ret;
    
    if(OK != (ret = ExLoadKernelDriversForDevice(rootDeviceId, &drivers, &driverCount)))
        return ret;
    
    if(1 != driverCount)
    {
        MmFreeKernelHeap(drivers);
        return IO_ROOT_DEVICE_INIT_FAILURE;
    }
    struct IoSubDeviceObject *rootSubDevice = NULL;
    if(OK != (ret = IoCreateSubDevice(drivers->this, 0, &rootSubDevice)))
    {
        MmFreeKernelHeap(drivers);
        return ret;
    }

    rootDevice.type = IO_DEVICE_TYPE_ROOT;
    rootDevice.flags = IO_DEVICE_FLAG_PERSISTENT;
    rootDevice.baseDevice = rootSubDevice;
    rootDevice.parent = NULL;

    rootDevice.deviceId = MmAllocateKernelHeap(CmStrlen(rootDeviceId));
    if(NULL == rootDevice.deviceId)
        goto exitIoInitDeviceManagerOnFailure;
    CmStrcpy(rootDevice.deviceId, rootDeviceId);

    struct IoDriverRp *rp;
    if(OK != IoCreateRp(&rp))
        goto exitIoInitDeviceManagerOnFailure;

    rp->code = IO_RP_ENUMERATE;
    rp->flags = IO_DRIVER_RP_FLAG_SYNCHRONOUS;
    if(OK != IoSendRp(rootSubDevice, NULL, rp))
        goto exitIoInitDeviceManagerOnFailure;
    
    MmFreeKernelHeap(rp);

    rootDevice.flags |= IO_DEVICE_FLAG_INITIALIZED;

    return OK;

exitIoInitDeviceManagerOnFailure:
    MmFreeKernelHeap(drivers);
    MmFreeKernelHeap(rootSubDevice);
    return IO_ROOT_DEVICE_INIT_FAILURE;
}

STATUS IoSendRp(struct IoSubDeviceObject *subDevice, struct IoDeviceObject *device, struct IoDriverRp *rp)
{
    ASSERT(rp);
    ASSERT(subDevice || device);
    if(NULL == subDevice)
    {
        subDevice = device->baseDevice;
        while(subDevice->attachedDevice) //find stack top
            subDevice = subDevice->attachedDevice;
    }
    if(NULL == subDevice->driverObject->dispatch)
        return DEVICE_NOT_AVAILABLE;

    rp->device = subDevice;
    STATUS status = subDevice->driverObject->dispatch(rp);
    if(OK != status)
        return status;

    if(rp->flags & IO_DRIVER_RP_FLAG_SYNCHRONOUS)
    {
        while(!rp->completed)
            ;
    }

    return status;
}

STATUS IoSendRpDown(struct IoSubDeviceObject *caller, struct IoDriverRp *rp)
{
    ASSERT(rp && caller);
    if(NULL != caller->attachedTo)
    {
        if(NULL != caller->attachedTo->driverObject->dispatch)
        {
            rp->device = caller->attachedTo;
            STATUS status = caller->attachedTo->driverObject->dispatch(rp);
            if(OK != status)
                return status;

            if(rp->flags & IO_DRIVER_RP_FLAG_SYNCHRONOUS)
            {
                while(!rp->completed)
                    ;
            }
            return status;
        }
    }
    return DEVICE_NOT_AVAILABLE;
}

STATUS IoSetDeviceDisplayedName(struct IoSubDeviceObject *device, char *name)
{
    if(NULL == device->mainDeviceObject)
        return OK;
    if(NULL != device->mainDeviceObject->name)
        MmFreeKernelHeap(device->mainDeviceObject->name);
    if(NULL == (device->mainDeviceObject->name = MmAllocateKernelHeap(CmStrlen(name))))
        return OUT_OF_RESOURCES;
    
    CmStrcpy(device->mainDeviceObject->name, name);
    return OK;
}

STATUS IoUpdateCompatibleDeviceIdList(struct IoDeviceObject *dev, char *id)
{
    if(NULL == id)
        return OK;
    for(uint8_t i = 0; i < IO_MAX_COMPATIBLE_DEVICE_IDS; i++)
    {
        if(NULL == dev->compatibleIds[i])
        {
            if(NULL == (dev->compatibleIds[i] = MmAllocateKernelHeap(CmStrlen(id))))
                return OUT_OF_RESOURCES;
            CmStrcpy(dev->compatibleIds[i], id);
            return OK;
        }
    }
    return OUT_OF_RESOURCES;
}