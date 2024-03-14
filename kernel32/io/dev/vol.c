#include "vol.h"
#include "assert.h"
#include "mm/heap.h"
#include "io/fs/vfs.h"
#include "ke/core/mutex.h"
#include "dev.h"
#include "common.h"

static struct
{
    struct IoVolumeNode *list;
    KeSpinlock lock;
} 
IoVolumeState = {.list = NULL, .lock = KeSpinlockInitializer};

// STATUS IoMountVolume(char *devPath, char *path)
// {
//     ASSERT(path && devPath);

//     STATUS status = OK;

//     struct IoVfsNode *devNode = IoVfsGetNodeByPath(devPath);
//     if(NULL == devNode)
//         return IO_FILE_NOT_FOUND;
    
//     if(IO_VFS_DEVICE != devNode->type)
//         return IO_BAD_FILE_TYPE;
    
//     if(NULL == devNode->device)
//         return IO_FILE_NOT_FOUND;

//     struct IoDeviceObject *dev = devNode->device;

//     if(IO_DEVICE_TYPE_DISK != dev->type)
//         return IO_NOT_DISK_DEVICE_FILE;
    
//     //check if there is an associated volume present already
//     if(NULL != dev->associatedVolume)
//         return IO_VOLUME_ALREADY_EXISTS;

//     struct IoVolumeNode *volume = MmAllocateKernelHeapZeroed(sizeof(*volume));
//     if(NULL == volume)
//         return OUT_OF_RESOURCES;
    
//     volume->pdo = dev;
//     dev->associatedVolume = volume;

//     KeAcquireSpinlock(&volumeListLock);
//     if(NULL == volumeList)
//         volumeList = volume;
//     else
//     {
//         struct IoVolumeNode *n = volumeList;
//         while(n->next)
//             n = n->next;
        
//         n->next = volume;
//         volume->previous = n;
//     }
//     KeReleaseSpinlock(&volumeListLock);

    
// }

STATUS IoRegisterVolume(struct IoDeviceObject *dev, IoDeviceFlags flags)
{
    ObLockObject(dev);
    if(dev->type != IO_DEVICE_TYPE_DISK)
    {
        ObUnlockObject(dev);
        return NOT_COMPATIBLE;
    }
    
    if(NULL != dev->associatedVolume)
    {
        ObUnlockObject(dev);
        return IO_VOLUME_ALREADY_EXISTS;
    }

    ObUnlockObject(dev);
    
    struct IoVolumeNode *node = MmAllocateKernelHeapZeroed(sizeof(*node));
    if(NULL == node)
        return OUT_OF_RESOURCES;
    
    ObInitializeObjectHeader(node);
    ObLockObject(node);
    ObLockObject(dev);
    
    node->pdo = dev;
    node->flags = flags;
    dev->associatedVolume = node;

    KeAcquireSpinlock(&(IoVolumeState.lock));
    if(NULL == IoVolumeState.list)
        IoVolumeState.list = node;
    else
    {
        struct IoVolumeNode *t = IoVolumeState.list;
        while(NULL != t->next)
        {
            t = t->next;
        }
        t->next = node;
        node->previous = t;
    }
    KeReleaseSpinlock(&(IoVolumeState.lock));

    ObUnlockObject(dev);
    ObUnlockObject(node);

    return OK;
}

STATUS IoSetVolumeSerialNumber(struct IoDeviceObject *dev, uint64_t serial)
{
    ObLockObject(dev);
    if(NULL == dev->associatedVolume)
    {
        ObUnlockObject(dev);
        return IO_VOLUME_NOT_REGISTERED;
    }

    struct IoVolumeNode *vol = dev->associatedVolume;
    ObUnlockObject(dev);

    ObLockObject(vol);
    vol->serialNumber = serial;
    ObUnlockObject(vol);
    return OK;
}

STATUS IoSetVolumeLabel(struct IoDeviceObject *dev, char *label)
{
    if(CmStrlen(label) > IO_VOLUME_MAX_LABEL_LENGTH)
        return IO_ILLEGAL_NAME;
    
    ObLockObject(dev);
    if(NULL == dev->associatedVolume)
    {
        ObUnlockObject(dev);
        return IO_VOLUME_NOT_REGISTERED;
    }

    struct IoVolumeNode *vol = dev->associatedVolume;
    ObUnlockObject(dev);

    ObLockObject(vol);
    CmStrncpy(vol->label, label, IO_VOLUME_MAX_LABEL_LENGTH);
    ObUnlockObject(vol);
    return OK;
}