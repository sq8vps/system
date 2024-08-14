#include "vol.h"
#include "assert.h"
#include "mm/heap.h"
#include "io/fs/vfs.h"
#include "ke/core/mutex.h"
#include "dev.h"
#include "common.h"
#include "ex/kdrv/fsdrv.h"

static struct
{
    struct IoVolumeNode *list;
    KeSpinlock lock;
} 
IoVolumeState = {.list = NULL, .lock = KeSpinlockInitializer};

STATUS IoMountVolume(char *devPath, char *mountPoint)
{
    ASSERT(mountPoint && devPath);

    //sanity check
    if(!CmCheckFileName(mountPoint))
        return IO_ILLEGAL_NAME;

    STATUS status = OK;

    //get VFS node associated with disk device
    struct IoVfsNode *devNode = IoVfsGetNode(devPath);
    if(NULL == devNode)
        return IO_FILE_NOT_FOUND;

    

    if(IO_VFS_DEVICE != devNode->type)
        status = IO_BAD_FILE_TYPE;
    
    if(NULL == devNode->device)
        status = IO_FILE_NOT_FOUND;
    
    if(OK != status)
    {
        
        return status;
    }

    //get underlying disk device node
    struct IoDeviceObject *dev = devNode->device;

    

    if(IO_DEVICE_TYPE_DISK != dev->type)
        status = IO_NOT_DISK_DEVICE_FILE;
    
    if(NULL == dev->associatedVolume)
        status = IO_VOLUME_NOT_REGISTERED;
    
    
    

    if(OK != status)
        return status;
    
    //load FS driver and ask to mount the FS 
    status = ExMountVolume(dev);

    if(OK == status)
    {
        if(NULL == dev->associatedVolume->fsdo)
            status = IO_VOLUME_NOT_REGISTERED;
    }
    
    if(OK != status)
    {
        return status;
    }

    if(IoVfsCheckIfNodeExists(mountPoint))
        return IO_FILE_ALREADY_EXISTS;
    
    struct IoVfsNode *mountPointNode = NULL;
    mountPointNode = IoVfsCreateNode(mountPoint);
    if(NULL == mountPointNode)
    {
        return OUT_OF_RESOURCES;
    }

    
    mountPointNode->type = IO_VFS_MOUNT_POINT;
    mountPointNode->fsType = IO_VFS_FS_PHYSICAL;
    mountPointNode->device = dev->associatedVolume->fsdo;

    dev->associatedVolume->mountPoint = mountPointNode;
    dev->referenceCount++;
    

    return IoVfsInsertNodeByPath(mountPointNode, "/");
}

STATUS IoRegisterVolume(struct IoDeviceObject *dev, IoDeviceFlags flags)
{
    
    if(dev->type != IO_DEVICE_TYPE_DISK)
    {
        
        return NOT_COMPATIBLE;
    }
    
    if(NULL != dev->associatedVolume)
    {
        
        return IO_VOLUME_ALREADY_EXISTS;
    }

    
    
    struct IoVolumeNode *node = MmAllocateKernelHeapZeroed(sizeof(*node));
    if(NULL == node)
        return OUT_OF_RESOURCES;
    
    ObInitializeObjectHeader(node);
    
    
    
    node->pdo = dev;
    node->flags = flags;
    dev->associatedVolume = node;

    PRIO prio = KeAcquireSpinlock(&(IoVolumeState.lock));
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
    KeReleaseSpinlock(&(IoVolumeState.lock), prio);

    
    

    return OK;
}

STATUS IoSetVolumeSerialNumber(struct IoDeviceObject *dev, uint64_t serial)
{
    
    if(NULL == dev->associatedVolume)
    {
        
        return IO_VOLUME_NOT_REGISTERED;
    }

    struct IoVolumeNode *vol = dev->associatedVolume;
    

    
    vol->serialNumber = serial;
    
    return OK;
}

STATUS IoSetVolumeLabel(struct IoDeviceObject *dev, char *label)
{
    if(CmStrlen(label) > IO_VOLUME_MAX_LABEL_LENGTH)
        return IO_ILLEGAL_NAME;
    
    
    if(NULL == dev->associatedVolume)
    {
        
        return IO_VOLUME_NOT_REGISTERED;
    }

    struct IoVolumeNode *vol = dev->associatedVolume;
    

    
    CmStrncpy(vol->label, label, IO_VOLUME_MAX_LABEL_LENGTH);
    
    return OK;
}

STATUS IoRegisterFilesystem(struct IoDeviceObject *disk, struct IoDeviceObject *fs, IoDeviceFlags volumeFlags)
{
    ASSERT(disk && fs);

    
    if(IO_DEVICE_TYPE_DISK != disk->type)
    {
        
        return IO_DEVICE_INCOMPATIBLE;
    }

    if(NULL == disk->associatedVolume)
    {
        
        return IO_VOLUME_NOT_REGISTERED;
    }

    
    if(IO_DEVICE_TYPE_FS != fs->type)
    {
        
        
        return IO_DEVICE_INCOMPATIBLE;
    }

    
    if(NULL != disk->associatedVolume->fsdo)
    {
        
        
        
        return IO_VOLUME_ALREADY_MOUNTED;
    }

    disk->associatedVolume->fsdo = fs;
    disk->associatedVolume->flags |= volumeFlags;
    fs->volumeNode = disk->associatedVolume;
    fs->volumeNode->flags |= IO_DEVICE_FLAG_FS_ASSOCIATED;

    
    
    
    return OK;
}