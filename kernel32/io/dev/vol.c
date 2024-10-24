#include "vol.h"
#include "assert.h"
#include "mm/heap.h"
#include "io/fs/vfs.h"
#include "io/fs/fs.h"
#include "ke/core/mutex.h"
#include "dev.h"
#include "common.h"
#include "ex/kdrv/fsdrv.h"
#include "ex/worker.h"
#include "ke/sched/sched.h"
#include "ex/db/db.h"
#include "ddk/disk.h"

static struct
{
    struct IoVolumeNode *list;
    KeSpinlock lock;
    char *mountPointDbPath;
} 
IoVolumeState = {.list = NULL, .lock = KeSpinlockInitializer, .mountPointDbPath = NULL};

struct IoAutoMountQueue
{
    struct IoVolumeNode *node;
    struct IoAutoMountQueue *next;
} static *IoAutoMountQueueHead = NULL;
static KeSpinlock IoAutoMountQueueLock = KeSpinlockInitializer;
static struct KeTaskControlBlock *IoAutoMountThread = NULL;

static void IoAutoMountWorker(void *unused);
static STATUS IoNotifyAutoMount(struct IoVolumeNode *node);

STATUS IoInitializeVolumeManager(void)
{
    STATUS status = OK;
    struct ExDbHandle *h = NULL;
    status = ExDbOpen(INITIAL_CONFIG_DATABASE, &h);
    if(OK != status)
        return status;
    
    char *t = NULL;
    if(OK != ExDbGetNextString(h, "MountPointDatabasePath", &t))
    {
        ExDbClose(h);
        return IO_FILE_NOT_FOUND;
    }

    IoVolumeState.mountPointDbPath = MmAllocateKernelHeap(CmStrlen(t) + 1);
    if(NULL == IoVolumeState.mountPointDbPath)
    {
        ExDbClose(h);
        return OUT_OF_RESOURCES;
    }

    CmStrcpy(IoVolumeState.mountPointDbPath, t);

    ExDbClose(h);

    return ExCreateKernelWorker("Drive auto mount", IoAutoMountWorker, NULL, &IoAutoMountThread);
}

STATUS IoMountVolumeByDevice(struct IoDeviceObject *dev, const char *mountPoint)
{
    ASSERT(mountPoint && dev);

    //sanity check
    if(!CmCheckPath(mountPoint))
        return IO_ILLEGAL_NAME;

    STATUS status = OK;

    if(IO_DEVICE_TYPE_DISK != dev->type)
        status = IO_NOT_DISK_DEVICE_FILE;
    
    if(NULL == dev->associatedVolume)
        status = IO_VOLUME_NOT_REGISTERED;
    
    if(OK != status)
        return status;
    
    //load FS driver and ask to mount the FS 
    status = ExMountVolume(dev->associatedVolume);

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
    
    struct IoVfsNode *node = NULL;
    node = IoVfsCreateNode(CmGetFileName(mountPoint));
    if(NULL == node)
    {
        return OUT_OF_RESOURCES;
    }
    
    node->type = IO_VFS_MOUNT_POINT;
    node->fsType = IO_VFS_FS_PHYSICAL;
    node->device = IoGetDeviceStackTop(dev->associatedVolume->fsdo);

    dev->associatedVolume->mountPoint = node;
    dev->referenceCount++;
    
    return IoVfsInsertNodeByFilePath(node, mountPoint);
}

STATUS IoMountVolume(const char *devPath, const char *mountPoint)
{
    ASSERT(mountPoint && devPath);

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

    return IoMountVolumeByDevice(dev, mountPoint);
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

    IoNotifyAutoMount(node);

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
    fs->node.volumeNode = disk->associatedVolume;
    fs->node.volumeNode->flags |= IO_DEVICE_FLAG_FS_ASSOCIATED;
    
    return OK;
}


static void IoAutoMountWorker(void *unused)
{
    while(1)
    {
        struct ExDbHandle *h = NULL;
        if(OK == ExDbOpen(IoVolumeState.mountPointDbPath, &h))
        {
            PRIO prio = KeAcquireSpinlock(&IoAutoMountQueueLock);
            while(NULL != IoAutoMountQueueHead)
            {
                struct IoAutoMountQueue *t = IoAutoMountQueueHead;
                IoAutoMountQueueHead = t->next;
                KeReleaseSpinlock(&IoAutoMountQueueLock, prio);
                
                char *signature = NULL;
                if(OK == DiskGetSignature(t->node->pdo, &signature))
                {
                    ExDbRewind(h);
                    char *mountPoint = NULL;
                    if(OK == ExDbGetNextString(h, signature, &mountPoint))
                    {
                        IoMountVolumeByDevice(t->node->pdo, mountPoint);
                    }
                }

                MmFreeKernelHeap(t);
                
                prio = KeAcquireSpinlock(&IoAutoMountQueueLock);
            }
            KeReleaseSpinlock(&IoAutoMountQueueLock, prio);
            ExDbClose(h);
        }

        KeEventSleep();
    }
}

static STATUS IoNotifyAutoMount(struct IoVolumeNode *node)
{
    struct IoAutoMountQueue *t = MmAllocateKernelHeap(sizeof(*t));
    if(NULL == t)
        return OUT_OF_RESOURCES;

    t->node = node;
    t->next = NULL;
    PRIO prio = KeAcquireSpinlock(&IoAutoMountQueueLock);
    if(NULL == IoAutoMountQueueHead)
    {
        IoAutoMountQueueHead = t;
    }
    else
    {
        struct IoAutoMountQueue *last = IoAutoMountQueueHead;
        while(last->next)
            last = last->next;
        last->next = t;
    }
    KeReleaseSpinlock(&IoAutoMountQueueLock, prio);
    KeWakeUpTask(IoAutoMountThread);
    return OK;
}