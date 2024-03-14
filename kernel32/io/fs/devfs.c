#include "devfs.h"
#include "assert.h"
#include "ke/core/mutex.h"
#include "io/dev/dev.h"
#include "mm/mm.h"
#include "common.h"

static struct
{
    struct IoVfsNode *root;
}
IoDevfsState = {.root = NULL};


// /**
//  * @brief Max number of registed device files for given type
// */
// #define MAX_DEVICES_OF_GIVEN_TYPE 256

// /**
//  * @brief A bitmap of registered device file names for each device type
// */
// static uint32_t IoDevfsNameUsage[__IO_DEVICE_TYPE_COUNT][MAX_DEVICES_OF_GIVEN_TYPE / 32];
// static KeSpinlock IoDevfsNameUsageLock[__IO_DEVICE_TYPE_COUNT];

// /**
//  * @brief A table of standard device name prefixes
// */
// static const char *IoDevfsDeviceNamePrefix[__IO_DEVICE_TYPE_COUNT] = 
// {
//     [IO_DEVICE_TYPE_NONE] = "dummy",
//     [IO_DEVICE_TYPE_BUS] = "bus",
//     [IO_DEVICE_TYPE_DISK] = "hd",
//     [IO_DEVICE_TYPE_STORAGE] = "stor",
//     [IO_DEVICE_TYPE_FS] = "fs",
//     [IO_DEVICE_TYPE_ROOT] = "root",
//     [IO_DEVICE_TYPE_OTHER] = "dev",
// };



// static char *IoMakeGenericDeviceFileName(struct IoDeviceObject *dev)
// {
//     ASSERT(dev);

//     uint32_t index;
//     char *name = NULL;
    
//     KeAcquireSpinlock(&IoDevfsNameUsageLock[dev->type]);
//     for(index = 0; index < MAX_DEVICES_OF_GIVEN_TYPE; index++)
//     {
//         if(0 == (IoDevfsNameUsage[dev->type][index / 32] & ((uint32_t)1 << (index % 32)))) //find free index
//         {
//             IoDevfsNameUsage[dev->type][index / 32] |= ((uint32_t)1 << (index % 32)); //mark as used
//             KeReleaseSpinlock(&IoDevfsNameUsageLock[dev->type]);
//             //there are 26 letter from a-z
//             //if required index is >= 26, then go with ba,bb,bc..., then baa,bab,bac... and finally ...zzzzx,...zzzzy,...zzzzz
//             uint8_t letters = index / 26 + 1;
//             uint32_t originalIndex = index;
//             uint32_t prefixLength = CmStrlen(IoDevfsDeviceNamePrefix[dev->type]);
//             name = MmAllocateKernelHeapZeroed(prefixLength + letters + 1);
//             if(NULL == name)
//             {
//                 KeAcquireSpinlock(&IoDevfsNameUsageLock[dev->type]);
//                 IoDevfsNameUsage[dev->type][index / 32] &= ~((uint32_t)1 << (index % 32)); //clear usage bit
//                 KeReleaseSpinlock(&IoDevfsNameUsageLock[dev->type]);
//                 return NULL;
//             }
//             //copy prefix
//             CmMemcpy(name, IoDevfsDeviceNamePrefix[dev->type], prefixLength);
//             //add letters
//             for(; letters > 0; letters--)
//             {
//                 name[prefixLength + letters - 1] = (index % 26) + 'a';
//                 index /= 26; 
//             }
            
//             return name;
//         }
//     }
//     KeReleaseSpinlock(&IoDevfsNameUsageLock[dev->type]);
//     return NULL;
// }

STATUS IoCreateDeviceFile(struct IoDeviceObject *dev, union IoVfsReference ref, IoVfsFlags flags, char *name)
{
    ASSERT(dev);
    ASSERT(IoDevfsState.root);
    ASSERT(name);

    if(IoVfsCheckIfNodeExists(IoDevfsState.root, name))
        return IO_FILE_ALREADY_EXISTS;

    struct IoVfsNode *node = IoVfsCreateNode(name);
    if(NULL == node)
    {
        return OUT_OF_RESOURCES;
    }
    node->type = IO_VFS_DEVICE;
    node->fsType = IO_VFS_FS_PHYSICAL;
    node->flags |= flags | IO_VFS_FLAG_NO_CACHE;

    IoVfsInsertNode(IoDevfsState.root, node);
    return OK;
}

STATUS IoInitDeviceFs(struct IoVfsNode *root)
{
    IoDevfsState.root = IoVfsCreateNode("dev");
    if(NULL == IoDevfsState.root)
        return OUT_OF_RESOURCES;
    
    IoDevfsState.root->flags = IO_VFS_FLAG_PERSISTENT;
    IoDevfsState.root->type = IO_VFS_DIRECTORY;
    IoDevfsState.root->fsType = IO_VFS_FS_VIRTUAL;
    
    IoVfsInsertNode(IoDevfsState.root, root);
    
    return OK;
}