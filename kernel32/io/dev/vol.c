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

    ObLockObject(devNode);

    if(IO_VFS_DEVICE != devNode->type)
        status = IO_BAD_FILE_TYPE;
    
    if(NULL == devNode->device)
        status = IO_FILE_NOT_FOUND;
    
    if(OK != status)
    {
        ObUnlockObject(devNode);
        return status;
    }

    //get underlying disk device node
    struct IoDeviceObject *dev = devNode->device;

    ObLockObject(dev);

    if(IO_DEVICE_TYPE_DISK != dev->type)
        status = IO_NOT_DISK_DEVICE_FILE;
    
    if(NULL == dev->associatedVolume)
        status = IO_VOLUME_NOT_REGISTERED;
    
    ObUnlockObject(dev);
    ObUnlockObject(devNode);

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

    ObLockObject(dev);
    mountPointNode->type = IO_VFS_MOUNT_POINT;
    mountPointNode->fsType = IO_VFS_FS_PHYSICAL;
    mountPointNode->device = dev->associatedVolume->fsdo;

    dev->associatedVolume->mountPoint = mountPointNode;
    dev->referenceCount++;
    ObUnlockObject(dev);

    return IoVfsInsertNodeByPath(mountPointNode, "/");
}

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

STATUS IoRegisterFilesystem(struct IoDeviceObject *disk, struct IoDeviceObject *fs, IoDeviceFlags volumeFlags)
{
    ASSERT(disk && fs);

    ObLockObject(disk);
    if(IO_DEVICE_TYPE_DISK != disk->type)
    {
        ObUnlockObject(disk);
        return IO_DEVICE_INCOMPATIBLE;
    }

    if(NULL == disk->associatedVolume)
    {
        ObUnlockObject(disk);
        return IO_VOLUME_NOT_REGISTERED;
    }

    ObLockObject(fs);
    if(IO_DEVICE_TYPE_FS != fs->type)
    {
        ObUnlockObject(fs);
        ObUnlockObject(disk);
        return IO_DEVICE_INCOMPATIBLE;
    }

    ObLockObject(disk->associatedVolume);
    if(NULL != disk->associatedVolume->fsdo)
    {
        ObUnlockObject(disk->associatedVolume);
        ObUnlockObject(fs);
        ObUnlockObject(disk);
        return IO_VOLUME_ALREADY_MOUNTED;
    }

    disk->associatedVolume->fsdo = fs;
    disk->associatedVolume->flags |= volumeFlags;
    fs->volumeNode = disk->associatedVolume;
    fs->volumeNode->flags |= IO_DEVICE_FLAG_FS_ASSOCIATED;

    ObUnlockObject(disk->associatedVolume);
    ObUnlockObject(fs);
    ObUnlockObject(disk);
    return OK;
}