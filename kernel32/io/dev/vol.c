#include "vol.h"
#include "assert.h"
#include "mm/heap.h"
#include "io/fs/vfs.h"
#include "ke/core/mutex.h"

static struct IoVolumeNode *volumeList = NULL;
static KeSpinlock volumeListLock = KeSpinlockInitializer;

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