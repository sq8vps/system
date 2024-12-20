#include "dev.h"
#include "mm/heap.h"
#include "rtl/string.h"
#include "assert.h"
#include "ex/worker.h"
#include "ke/sched/sched.h"
#include "io/dev/vol.h"
#include "ex/kdrv/kdrv.h"
#include "io/dev/rp.h"
#include "io/fs/vfs.h"

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

static struct IoEnumerationQueue *IoEnumerationRetryQueueHead = NULL;
static KeSpinlock IoEnumerationRetryQueueLock = KeSpinlockInitializer;

static void IoDeviceEnumeratorWorker(void *unused);

STATUS IoCreateDevice(
    struct ExDriverObject *driver, 
    enum IoDeviceType type,
    enum IoDeviceFlags flags, 
    struct IoDeviceObject **device)
{
    ASSERT(driver);
    *device = ObCreateKernelObject(OB_DEVICE);
    if(NULL == *device)
        return OUT_OF_RESOURCES;
    
    
    (*device)->driverObject = driver;
    (*device)->flags = flags;
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
    
    ObDestroyObject(device);
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
    struct IoDeviceNode *node = ObCreateKernelObject(OB_DEVICE_NODE);
    if(NULL == node)
        return OUT_OF_RESOURCES;

    
    node->standalone = false;
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

    return IoNotifyDeviceEnumerator(node);
}

STATUS IoRegisterStandaloneDevice(struct IoDeviceObject *dev)
{
    ASSERT(dev);

    if(!(dev->flags & IO_DEVICE_FLAG_STANDALONE))
        return BAD_PARAMETER;

    //create device node
    struct IoDeviceNode *node = ObCreateKernelObject(OB_DEVICE_NODE);
    if(NULL == node)
        return OUT_OF_RESOURCES;
    
    node->standalone = true;
    node->next = node;
    node->previous = node;

    //attach device to a node
    dev->node.deviceNode = node;
    //store device pointer as both BDO and MDO
    node->bdo = dev;
    node->mdo = dev;

    node->parent = NULL;

    return IoNotifyDeviceEnumerator(node);
}

STATUS IoDestroyDeviceNode(struct IoDeviceNode *node)
{
    ObDestroyObject(node);
    return OK;
}


STATUS IoBuildDeviceStack(struct IoDeviceNode *node)
{
    ASSERT(node);

    STATUS ret;
    char *deviceId, **compatibleIds;
    
    if(OK != (ret = IoGetDeviceId(node->bdo, &deviceId, (char***)&compatibleIds)))
    {
        node->status = IO_DEVICE_STATUS_INITIALIZATION_FAILED;
        return ret;
    }

    struct ExDriverObjectList *drivers = NULL;
    uint16_t driverCount = 0;

    //find and load required drivers
    if(OK != (ret = ExLoadKernelDriversForDevice(deviceId, compatibleIds, &drivers, &driverCount)))
    {
        node->status = IO_DEVICE_STATUS_INITIALIZATION_FAILED;
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
            node->status = IO_DEVICE_STATUS_INITIALIZATION_FAILED;
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
    node->status = IO_DEVICE_STATUS_READY;
    MmFreeKernelHeap(drivers);
    return OK;
}

STATUS IoInitDeviceManager(char *rootDeviceId)
{
    ASSERT(rootDeviceId);
    STATUS ret = OK;

    struct ExDriverObjectList *drivers = NULL;
    uint16_t driverCount = 0;
    
    ret = ExCreateKernelWorker(IoDeviceEnumeratorWorker, NULL, &IoEnumerationThread);
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
        return ROOT_DEVICE_INIT_FAILURE;
    }
    struct IoDeviceObject *rootBaseDevice = NULL;
    if(OK != (ret = IoCreateDevice(drivers->this, IO_DEVICE_TYPE_ROOT, 0, &rootBaseDevice)))
    {
        MmFreeKernelHeap(drivers);
        return ret;
    }

    rootNode.bdo = rootBaseDevice;
    rootNode.mdo = rootBaseDevice;
    rootNode.parent = NULL;
    rootNode.child = NULL;
    rootNode.next = NULL;
    rootNode.previous = NULL;
    rootNode.standalone = true;
    rootBaseDevice->node.deviceNode = &rootNode;
    rootBaseDevice->flags |= IO_DEVICE_FLAG_ENUMERATION_CAPABLE | IO_DEVICE_FLAG_PERSISTENT | IO_DEVICE_FLAG_STANDALONE;

    rootNode.status = IO_DEVICE_STATUS_READY;

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

STATUS IoPerfromIoctl(struct IoDeviceObject *dev, uint32_t ioctl, void *dataIn, void **dataOut)
{
    ASSERT(dev);

    STATUS status = OK;

    struct IoRp *rp = IoCreateRp();
    if(NULL == rp)
        return OUT_OF_RESOURCES;
    
    rp->device = dev;
    rp->code = IO_RP_IOCTL;
    rp->payload.ioctl.code = ioctl;
    rp->payload.ioctl.data = dataIn;
    status = IoSendRp(dev, rp);
    if(OK == status)
    {
        IoWaitForRpCompletion(rp);
        status = rp->status;
        if((OK == status) && (NULL != dataOut))
        {
            *dataOut = rp->payload.ioctl.data;
        }
    }

    IoFreeRp(rp);
    
    return status;       
}

STATUS IoGetDeviceForFile(struct IoVfsNode *node, struct IoDeviceObject **dev)
{
    if(IO_VFS_DEVICE != node->type)
        return DEVICE_NOT_AVAILABLE;
    
    if(NULL != dev)
        *dev = node->device;
    
    return OK;
}

static bool IoBuildDeviceStackAndEnumerate(struct IoEnumerationQueue *t)
{
    STATUS status = OK;
    if(t->node->standalone || (OK == IoBuildDeviceStack(t->node)))
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
                    t->node->statusFlags |= IO_DEVICE_STATUS_FLAG_ENUMERATION_FAILED;
                }
                
                IoFreeRp(rp);
            }
        }
        return true;
    }
    else
        return false;
}

void IoRetryBuildDeviceStackAndEnumerate(void)
{
    PRIO prio = KeAcquireSpinlock(&IoEnumerationRetryQueueLock);
    while(NULL != IoEnumerationRetryQueueHead)
    {
        struct IoEnumerationQueue *t = IoEnumerationRetryQueueHead;
        IoEnumerationRetryQueueHead = t->next;
        KeReleaseSpinlock(&IoEnumerationRetryQueueLock, prio);
        
        IoBuildDeviceStackAndEnumerate(t);
        MmFreeKernelHeap(t);
        
        prio = KeAcquireSpinlock(&IoEnumerationRetryQueueLock);
    }
    KeReleaseSpinlock(&IoEnumerationRetryQueueLock, prio); 
}

static void IoDeviceEnumeratorWorker(void *context)
{
    UNUSED(context);
    while(1)
    {
        PRIO prio = KeAcquireSpinlock(&IoEnumerationQueueLock);
        while(NULL != IoEnumerationQueueHead)
        {
            struct IoEnumerationQueue *t = IoEnumerationQueueHead;
            IoEnumerationQueueHead = t->next;
            KeReleaseSpinlock(&IoEnumerationQueueLock, prio);
            
            if(!IoBuildDeviceStackAndEnumerate(t))
            {
                PRIO prio = KeAcquireSpinlock(&(IoEnumerationRetryQueueLock));
                if(NULL != IoEnumerationRetryQueueHead)
                {
                    struct IoEnumerationQueue *s = IoEnumerationRetryQueueHead;
                    while(NULL != s->next)
                        s = s->next;
                    s->next = t;
                }
                else
                {
                    IoEnumerationRetryQueueHead = t;
                }
                t->next = NULL;
                KeReleaseSpinlock(&(IoEnumerationRetryQueueLock), prio);
            }
            else
                MmFreeKernelHeap(t);
            
            prio = KeAcquireSpinlock(&IoEnumerationQueueLock);
        }
        KeReleaseSpinlock(&IoEnumerationQueueLock, prio); 
        
        KeWaitForWakeUp();
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