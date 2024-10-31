#include "dev.h"
#include "mm/heap.h"
#include "rtl/string.h"
#include "assert.h"
#include "ex/worker.h"
#include "ke/sched/sched.h"
#include "io/dev/vol.h"
#include "ex/kdrv/kdrv.h"
#include "io/dev/rp.h"

//root device (ACPI or MP) node
static struct IoDeviceNode rootNode = {};

struct IoEnumerationQueue
{
    struct IoDeviceNode *node;
    struct IoEnumerationQueue *next;
};
static struct IoEnumerationQueue *IoEnumerationQueueHead = NULL;
static KeSpinlock IoEnumerationQueueLock = KeSpinlockInitializer;
static struct KeTaskControlBlock *IoEnumerationThread = NULL;

static void IoDeviceEnumeratorWorker(void *unused);

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
    
    ObInitializeObjectHeader(*device);
    
    (*device)->driverObject = driver;
    (*device)->flags |= flags;
    (*device)->type = type;
    
    //update list of created devices
    (*device)->nextDevice = driver->deviceObject;
    driver->deviceObject = *device;

    return OK;
}

STATUS IoDestroyDevice(struct IoDeviceObject *device)
{
    if(device->attachedTo || device->attachedDevice || device->node.deviceNode || device->node.volumeNode || (device->flags & IO_DEVICE_FLAG_PERSISTENT))
        return OPERATION_NOT_ALLOWED;
    
    MmFreeKernelHeap(device);
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
    
    ObInitializeObjectHeader(node);
        
    node->next = node;
    node->previous = node;

    //attach BDO to a node
    bdo->node.deviceNode = node;
    //store BDO pointer
    node->bdo = bdo;

    //store parent node
    node->parent = enumerator->node.deviceNode;

    //update parent node child list
    if(NULL == enumerator->node.deviceNode->child)
    {
        enumerator->node.deviceNode->child = node;
    }
    else
    {
        node->next = enumerator->node.deviceNode->child;
        node->previous = enumerator->node.deviceNode->child->previous;
        enumerator->node.deviceNode->child->previous->next = node;
        enumerator->node.deviceNode->child->previous = node;
    }

    node->flags |= IO_DEVICE_FLAG_INITIALIZED;

    return IoNotifyDeviceEnumerator(node);
}



STATUS IoBuildDeviceStack(struct IoDeviceNode *node)
{
    ASSERT(node);

    STATUS ret;
    char *deviceId, **compatibleIds;
    
    if(OK != (ret = IoGetDeviceId(node->bdo, &deviceId, (char***)&compatibleIds)))
    {
        node->flags |= IO_DEVICE_FLAG_INITIALIZATION_FAILURE;
        return ret;
    }

    struct ExDriverObjectList *drivers = NULL;
    uint16_t driverCount = 0;

    //find and load required drivers
    if(OK != (ret = ExLoadKernelDriversForDevice(deviceId, compatibleIds, &drivers, &driverCount)))
    {
        node->flags |= IO_DEVICE_FLAG_INITIALIZATION_FAILURE;
        MmFreeKernelHeap(drivers);
        return ret;
    }

    RtlFreeStringTable(compatibleIds, IO_MAX_COMPATIBLE_DEVICE_IDS);
    MmFreeKernelHeap(deviceId);

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
        //store main device object pointer
        //since the currently processed driver should create main device object,
        //get the topmost device from the stack
        if(d->isMain)
            node->mdo = IoGetDeviceStackTop(node->bdo);
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
    
    ret = ExCreateKernelWorker("Device enumeration", IoDeviceEnumeratorWorker, NULL, &IoEnumerationThread);
    if(OK != ret)
        return ret;

    ret = IoInitializeVolumeManager();
    if(OK != ret)
        return ret;
    
    if(OK != (ret = ExLoadKernelDriversForDevice(rootDeviceId, NULL, &drivers, &driverCount)))
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
    rootBaseDevice->node.deviceNode = &rootNode;
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

    rp->task = KeGetCurrentTask();
    rp->device = dev;
    return dev->driverObject->dispatch(rp);
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

STATUS IoGetDeviceId(struct IoDeviceObject *dev, char **deviceId, char ***compatibleIds)
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
    status = IoSendRp(dev, rp);
    if(OK == status)
    {
        IoWaitForRpCompletion(rp);
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
    rp->payload.configSpace.offset = offset;
    rp->size = size;
    rp->payload.configSpace.buffer = NULL;
    status = IoSendRp(dev, rp);
    if(OK == status)
    {
        IoWaitForRpCompletion(rp);
        status = rp->status;
        if(OK == status)
            *buffer = rp->payload.configSpace.buffer;
    }
    
    if(OK != status)
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
    rp->payload.configSpace.offset = offset;
    rp->size = size;
    rp->payload.configSpace.buffer = buffer;
    status = IoSendRp(dev, rp);
    if(OK == status)
    {
        IoWaitForRpCompletion(rp);
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
    status = IoSendRp(dev, rp);
    if(OK == status)
    {
        IoWaitForRpCompletion(rp);
        status = rp->status;
        if(OK == status)
        {
            *res = rp->payload.resource.res;
            *count = rp->payload.resource.count;
        }
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
    status = IoSendRp(dev, rp);
    if(OK == status)
    {
        IoWaitForRpCompletion(rp);
        status = rp->status;
        if(OK == status)
        {
            *type = rp->payload.location.type;
            *location = rp->payload.location.id;
        }
    }

    IoFreeRp(rp);
    
    return status;       
}

static void IoDeviceEnumeratorWorker(void *unused)
{
    STATUS status;
    while(1)
    {
        PRIO prio = KeAcquireSpinlock(&IoEnumerationQueueLock);
        while(NULL != IoEnumerationQueueHead)
        {
            struct IoEnumerationQueue *t = IoEnumerationQueueHead;
            IoEnumerationQueueHead = t->next;
            KeReleaseSpinlock(&IoEnumerationQueueLock, prio);
            
            if((NULL != t->node->mdo)
                || (OK == IoBuildDeviceStack(t->node)))
            {
                if((IO_DEVICE_TYPE_BUS == t->node->mdo->type)
                    || (t->node->mdo->flags & IO_DEVICE_FLAG_ENUMERATION_CAPABLE))
                {
                    struct IoRp *rp = IoCreateRp();
                    if(NULL != rp)
                    {
                        rp->code = IO_RP_ENUMERATE;
                        status = IoSendRp(t->node->mdo, rp);
                        if(OK == status)
                        {
                            IoWaitForRpCompletion(rp);
                            status = rp->status;
                        }

                        if(OK != status)
                        {
                            t->node->flags |= IO_DEVICE_FLAG_INITIALIZATION_FAILURE;
                        }
                        
                        IoFreeRp(rp);
                    }
                }
            }
            MmFreeKernelHeap(t);
            
            prio = KeAcquireSpinlock(&IoEnumerationQueueLock);
        }
        KeReleaseSpinlock(&IoEnumerationQueueLock, prio); 
        
        KeEventSleep();
    }
}

STATUS IoNotifyDeviceEnumerator(struct IoDeviceNode *node)
{
    struct IoEnumerationQueue *t = MmAllocateKernelHeap(sizeof(*t));
    if(NULL == t)
        return OUT_OF_RESOURCES;

    t->node = node;
    t->next = NULL;
    PRIO prio = KeAcquireSpinlock(&IoEnumerationQueueLock);
    if(NULL == IoEnumerationQueueHead)
    {
        IoEnumerationQueueHead = t;
    }
    else
    {
        struct IoEnumerationQueue *last = IoEnumerationQueueHead;
        while(last->next)
            last = last->next;
        last->next = t;
    }
    KeReleaseSpinlock(&IoEnumerationQueueLock, prio);
    KeWakeUpTask(IoEnumerationThread);
    return OK;
}