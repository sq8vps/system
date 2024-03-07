#include "dev.h"
#include "mm/heap.h"
#include "common.h"
#include "assert.h"
#include "enumeration.h"
#include "ke/sched/sched.h"

//root device (ACPI or MP) node
static struct IoDeviceNode rootNode = {};

STATUS IoCreateDevice(
    struct ExDriverObject *driver, 
    enum IoDeviceType type,
    IoDeviceFlags flags, 
    struct IoDeviceObject **device)
{
    ASSERT(driver);
    *device = MmAllocateKernelHeapZeroed(sizeof(**device));
    if(NULL == *device)
        return OUT_OF_RESOURCES;
    
    (*device)->driverObject = driver;
    (*device)->flags |= flags;
    (*device)->type = type;
    
    //update list of created devices
    (*device)->nextDevice = driver->deviceObject;
    driver->deviceObject = *device;

    return OK;
}


struct IoDeviceObject* IoAttachDevice(struct IoDeviceObject *attachee, struct IoDeviceObject *destination)
{
    ASSERT(attachee && destination);
    while(destination->attachedDevice)
    {
        destination = destination->attachedDevice;
    }
    destination->attachedDevice = attachee;
    attachee->attachedTo = destination;
    attachee->node = destination->node;
    return destination;
}

STATUS IoRegisterDevice(struct IoDeviceObject *bdo, struct IoDeviceObject *enumerator)
{
    ASSERT(bdo && enumerator);

    //create device node
    struct IoDeviceNode *node = MmAllocateKernelHeapZeroed(sizeof(*node));
    if(NULL == node)
        return OUT_OF_RESOURCES;
        
    node->next = node;
    node->previous = node;

    //attach BDO to a node
    bdo->node = node;
    //store BDO pointer
    node->bdo = bdo;

    //store parent node
    node->parent = enumerator->node;

    //update parent node child list
    if(NULL == enumerator->node->child)
    {
        enumerator->node->child = node;
    }
    else
    {
        node->next = enumerator->node->child;
        node->previous = enumerator->node->child->previous;
        enumerator->node->child->previous->next = node;
        enumerator->node->child->previous = node;
    }

    node->flags |= IO_DEVICE_FLAG_INITIALIZED;

    return IoNotifyDeviceEnumerator(node);
}



STATUS IoBuildDeviceStack(struct IoDeviceNode *node)
{
    ASSERT(node);

    STATUS ret;
    char *deviceId, *compatibleIds[IO_MAX_COMPATIBLE_DEVICE_IDS];
    
    if(OK != (ret = IoGetDeviceId(node->bdo, &deviceId, (char***)&compatibleIds)))
    {
        node->flags |= IO_DEVICE_FLAG_INITIALIZATION_FAILURE;
        return ret;
    }

    struct ExDriverObjectList *drivers = NULL;
    uint16_t driverCount = 0;

    //find and load required drivers
    if(OK != (ret = ExLoadKernelDriversForDevice(deviceId, &drivers, &driverCount)))
    {
        node->flags |= IO_DEVICE_FLAG_INITIALIZATION_FAILURE;
        MmFreeKernelHeap(drivers);
        return ret;
    }

    //invoke AddDevice routine for all drivers to form a device stack
    struct ExDriverObjectList *d = drivers;
    for(uint16_t i = 0; i < driverCount; i++)
    {
        if(OK != (ret = d->this->addDevice(d->this, node->bdo)))
        {
            node->flags |= IO_DEVICE_FLAG_INITIALIZATION_FAILURE;
            MmFreeKernelHeap(drivers);
            return ret;
        }
        //TODO: remove!!!!!!
        if(i == 0)
            node->mdo = node->bdo->attachedDevice;
        d = d->next;
    }
    node->flags |= IO_DEVICE_FLAG_READY_TO_RUN;
    MmFreeKernelHeap(drivers);
    return OK;
}

STATUS IoInitDeviceManager(char *rootDeviceId)
{
    ASSERT(rootDeviceId);
    STATUS ret = OK;

    ret = IoInitializeRpCache();
    if(OK != ret)
        return ret;

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
    struct IoDeviceObject *rootBaseDevice = NULL;
    if(OK != (ret = IoCreateDevice(drivers->this, IO_DEVICE_TYPE_ROOT, 0, &rootBaseDevice)))
    {
        MmFreeKernelHeap(drivers);
        return ret;
    }

    rootNode.flags = IO_DEVICE_FLAG_PERSISTENT;
    rootNode.bdo = rootBaseDevice;
    rootNode.mdo = rootBaseDevice;
    rootNode.parent = NULL;
    rootNode.child = NULL;
    rootNode.next = NULL;
    rootNode.previous = NULL;
    rootBaseDevice->node = &rootNode;
    rootBaseDevice->flags |= IO_DEVICE_FLAG_ENUMERATION_CAPABLE;

    rootNode.flags |= IO_DEVICE_FLAG_INITIALIZED;

    IoNotifyDeviceEnumerator(&rootNode);

    return OK;
}

struct IoDeviceObject* IoGetDeviceStackTop(struct IoDeviceObject *dev)
{
    ASSERT(dev);
    while(dev->attachedDevice) //find stack top
        dev = dev->attachedDevice;
    return dev;
}

STATUS IoSendRp(struct IoDeviceObject *dev, struct IoRp *rp)
{
    ASSERT(rp);
    ASSERT(dev);
    if(NULL == dev->driverObject->dispatch)
        return DEVICE_NOT_AVAILABLE;

    if(rp->flags & IO_DRIVER_RP_FLAG_SYNCHRONOUS)
    {
        rp->task = KeGetCurrentTask();
        KeBlockTask(rp->task, TASK_BLOCK_IO);
    }

    rp->device = dev;
    STATUS status = dev->driverObject->dispatch(rp);
    if(OK != status)
    {
        //TODO: this is really not elegant
        if(rp->flags & IO_DRIVER_RP_FLAG_SYNCHRONOUS)
            KeUnblockTask(rp->task);
        return status;
    }

    if(rp->flags & IO_DRIVER_RP_FLAG_SYNCHRONOUS)
        KeTaskYield();

    return OK;
}

STATUS IoSendRpDown(struct IoRp *rp)
{
    ASSERT(rp);
    struct IoDeviceObject *dev = rp->device->attachedTo;
    if(NULL != dev)
    {
        if(NULL != dev->driverObject->dispatch)
        {
            return IoSendRp(dev, rp);
        }
    }
    return DEVICE_NOT_AVAILABLE;
}

STATUS IoGetDeviceId(struct IoDeviceObject *dev, char **deviceId, char **compatibleIds[IO_MAX_COMPATIBLE_DEVICE_IDS])
{
    ASSERT(dev);
    *deviceId = NULL;
    *compatibleIds = NULL;
    STATUS status;
    struct IoRp *rp = IoCreateRp();
    if(NULL == rp)
        return OUT_OF_RESOURCES;
    rp->device = dev;
    rp->code = IO_RP_GET_DEVICE_ID;
    rp->flags |= IO_DRIVER_RP_FLAG_SYNCHRONOUS;
    status = IoSendRp(dev, rp);
    if(OK == status)
    {
        if(OK == rp->status)
        {
            *deviceId = rp->payload.deviceId.mainId;
            *compatibleIds = rp->payload.deviceId.compatibleId;
        }
        else
        {
            status = rp->status;
        }
    }
    IoFreeRp(rp);
    return status;
}

STATUS IoReadConfigSpace(struct IoDeviceObject *dev, uint64_t offset, uint64_t size, void **buffer)
{
    ASSERT(dev && buffer);

    *buffer = NULL;

    STATUS status = OK;

    struct IoRp *rp = IoCreateRp();
    if(NULL == rp)
        return OUT_OF_RESOURCES;
    
    rp->device = dev;
    rp->code = IO_RP_GET_CONFIG_SPACE;
    rp->flags |= IO_DRIVER_RP_FLAG_SYNCHRONOUS;
    rp->payload.configSpace.offset = offset;
    rp->size = size;
    rp->payload.configSpace.buffer = NULL;
    status = IoSendRp(dev, rp);
    if(OK == status)
    {
        *buffer = rp->payload.configSpace.buffer;
        status = rp->status;
    }
    else if(NULL != rp->payload.configSpace.buffer)
        MmFreeKernelHeap(rp->payload.configSpace.buffer);

    IoFreeRp(rp);

    return status;
}

STATUS IoWriteConfigSpace(struct IoDeviceObject *dev, uint64_t offset, uint64_t size, void *buffer)
{
    ASSERT(dev && buffer);

    STATUS status = OK;

    struct IoRp *rp = IoCreateRp();
    if(NULL == rp)
        return OUT_OF_RESOURCES;
    
    rp->device = dev;
    rp->code = IO_RP_SET_CONFIG_SPACE;
    rp->flags |= IO_DRIVER_RP_FLAG_SYNCHRONOUS;
    rp->payload.configSpace.offset = offset;
    rp->size = size;
    rp->payload.configSpace.buffer = buffer;
    status = IoSendRp(dev, rp);
    if(OK == status)
    {
        status = rp->status;
    }

    IoFreeRp(rp);
    
    return status;
}

STATUS IoGetDeviceResources(struct IoDeviceObject *dev, struct IoDeviceResource **res, uint32_t *count)
{
    ASSERT(dev && res);

    *count = 0;

    STATUS status = OK;

    struct IoRp *rp = IoCreateRp();
    if(NULL == rp)
        return OUT_OF_RESOURCES;
    
    rp->device = dev;
    rp->code = IO_RP_GET_DEVICE_RESOURCES;
    rp->flags |= IO_DRIVER_RP_FLAG_SYNCHRONOUS;
    status = IoSendRp(dev, rp);
    if(OK == status)
    {
        status = rp->status;
        *res = rp->payload.resource.res;
        *count = rp->payload.resource.count;
    }

    IoFreeRp(rp);
    
    return status;    
}

STATUS IoGetDeviceLocation(struct IoDeviceObject *dev, enum IoBusType *type, union IoBusId *location)
{
    ASSERT(dev && type && location);

    STATUS status = OK;

    struct IoRp *rp = IoCreateRp();
    if(NULL == rp)
        return OUT_OF_RESOURCES;
    
    rp->device = dev;
    rp->code = IO_RP_GET_DEVICE_LOCATION;
    rp->flags |= IO_DRIVER_RP_FLAG_SYNCHRONOUS;
    status = IoSendRp(dev, rp);
    if(OK == status)
    {
        status = rp->status;
        *type = rp->payload.location.type;
        *location = rp->payload.location.id;
    }

    IoFreeRp(rp);
    
    return status;       
}