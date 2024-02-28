#include "mount.h"
#include "vfs.h"
#include "assert.h"
#include "common.h"
#include "mm/heap.h"

STATUS IoMountVolume(char *device, char *mountPointName)
{
    ASSERT(device && mountPointName);

    for(uint32_t i = 0; i < CmStrlen(device); i++)
    {
        if(!(IS_ALPHANUMERIC(device[i]) || ('_' == device[i]) || ('-' == device[i]) || ('/' == device[i])))
            return IO_ILLEGAL_NAME;
    }
    
    for(uint32_t i = 0; i < CmStrlen(mountPointName); i++)
    {
        if(!(IS_ALPHANUMERIC(mountPointName[i]) || ('_' == mountPointName[i]) || ('-' == mountPointName[i])))
            return IO_ILLEGAL_NAME;
    }

    //get device to mount
    struct IoVfsNode *dev = IoVfsGetNodeByPath(device);
    if(NULL == dev)
    {
        PRINT("Device %s not found\n", device);
        return IO_FILE_NOT_FOUND;
    }
    
    if(IO_VFS_DISK != dev->type)
        return IO_NOT_DISK_DEVICE_FILE;
    
    //prepare mount point path
    char *mountPointPath = MmAllocateKernelHeap(CmStrlen(mountPointName) + 1);
    if(NULL == mountPointPath)
        return OUT_OF_RESOURCES;

    CmStrcpy(&mountPointPath[1], mountPointName);
    mountPointPath[0] = '/';
    PRINT("Mounted volume %s to %s\n", device, mountPointPath);
    
    STATUS ret = IoVfsCreateLink(mountPointPath, device, IO_VFS_FLAG_MOUNT_POINT);
    MmFreeKernelHeap(mountPointPath);
    return ret;
}

STATUS IoUnmountVolume(char *mountPoint)
{
    return NOT_IMPLEMENTED;
}