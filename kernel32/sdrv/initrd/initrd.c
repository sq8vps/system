#include "initrd.h"
#include "common.h"
#include "io/fs/vfs.h"
#include "io/fs/mount.h"

#define INITRD_DEVICE_NAME "initrd"

static struct InitrdHeader *header = NULL;

STATUS InitrdInit(uintptr_t address)
{
    header = (struct InitrdHeader*)address;
    if(0 != CmStrcmp(header->magic, INITRD_MAGIC_NUMBER))
    {
        header = NULL;
        return INITRD_INIT_FAILURE;
    }

    struct IoVfsNode *device = IoVfsCreateNode(INITRD_DEVICE_NAME);
    if(NULL == device)
        return OUT_OF_RESOURCES;

    device->type = IO_VFS_DISK;
    device->majorType = IO_VFS_FS_INITRD;
    device->flags = IO_VFS_FLAG_NO_CACHE | IO_VFS_FLAG_READ_ONLY;
    
    STATUS ret = OK; 
    if(OK != (ret = IoVfsInsertNodeByPath(device, "/dev/")))
        return ret;
    
    if(OK != (ret = IoMountVolume("/dev/" INITRD_DEVICE_NAME, INITRD_MOUNT_POINT)))
        return ret;

    return OK;
}

void InitrdDeinit(void)
{
    header = NULL;
}

uintptr_t InitrdGetFileReference(char *name, uint64_t *size)
{
    if(NULL == header)
        return 0;
    
    struct InitrdFileHeader *file = (struct InitrdFileHeader*)((uintptr_t)header + header->firstFileOffset);
    while(1)
    {
        if(0 == CmStrncmp(name, file->name, INITRD_MAX_FILE_NAME_LENGTH))
        {
            *size = file->size;
            return (uintptr_t)(file);
        }
        if(0 == file->nextFileOffset)
            break;

        file = (struct InitrdFileHeader*)((uintptr_t)file + file->nextFileOffset);
    }
    return 0;
}

uint64_t InitrdReadFile(uintptr_t reference, void *buffer, uint64_t size, uint64_t offset)
{
    if((NULL == header) || (0 == reference) || (NULL == buffer) || (0 == size))
        return 0;
    
    struct InitrdFileHeader *file = (struct InitrdFileHeader*)reference;
    
    if(offset >= file->size)
    {
        return 0;
    }
    else if((offset + size) >= file->size)
        size = file->size - offset;
    
    CmMemcpy(buffer, (uint8_t*)((uintptr_t)(file + 1) + (uintptr_t)offset), size);
    return size;
}

