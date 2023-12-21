#include "fs.h"
#include "vfs.h"
#include "ke/task/task.h"
#include "mm/heap.h"
#include "assert.h"
#include "common.h"

#define IO_FS_DEFAULT_FILE_HANDLE_LIMIT 1024
static uint32_t fileHandleLimit = IO_FS_DEFAULT_FILE_HANDLE_LIMIT;

static STATUS open(char *file, IoFileOpenMode mode, IoFileFlags flags, struct IoFileHandle **handle)
{
    *handle = NULL;

    struct IoVfsNode *fileNode = IoVfsGetNode(file);
    if(NULL == fileNode)
        return IO_FILE_NOT_FOUND;

    if(IO_VFS_FILE != fileNode->type)
        return IO_BAD_FILE_TYPE;

    if((mode & (IO_FILE_WRITE | IO_FILE_WRITE_ATTRIBUTES | IO_FILE_APPEND)) && (fileNode->flags & IO_VFS_FLAG_READ_ONLY))
        return IO_FILE_READ_ONLY;

    if(fileNode->flags & IO_VFS_FLAG_LOCK)
    {
        //if(flags & IO_FILE_FLAG_NO_WAIT)
            return IO_FILE_LOCKED;
    }

    //handle for newly opened file
    *handle = MmAllocateKernelHeap(sizeof(struct IoFileHandle));
    if(NULL == *handle)
         return OUT_OF_RESOURCES;

    CmMemset(*handle, 0, sizeof(**handle));
    (*handle)->type.fileHandle.node = fileNode;
    (*handle)->type.fileHandle.mode = mode;
    (*handle)->type.fileHandle.flags = flags;
    return OK;
}

STATUS IoOpenFile(char *file, IoFileOpenMode mode, IoFileFlags flags, struct KeTaskControlBlock *task, int *handleNumber)
{
    ASSERT(file && handleNumber && task);
    if(task->parent)
        task = task->parent;

    if(task->files.openFileCount > fileHandleLimit)
    {
        *handleNumber = -1;
        return IO_FILE_COUNT_LIMIT_REACHED;
    }

    struct IoFileHandle *handle = NULL;
    STATUS ret = OK;
    if(OK != (ret = open(file, mode, flags, &handle)))
        return ret;

    //handle for free handle range entry
    struct IoFileHandle *freeHandle;
    if(NULL == task->files.fileList)
    {
        freeHandle = MmAllocateKernelHeap(sizeof(struct IoFileHandle));
        if(NULL == freeHandle)
        {
            *handleNumber = -1;
            MmFreeKernelHeap(handle);
            return OUT_OF_RESOURCES;
        }
        freeHandle->free = true;
        freeHandle->previous = NULL;
        freeHandle->type.freeHandleRange.first = 1;
        freeHandle->type.freeHandleRange.last = fileHandleLimit + 1;
        freeHandle->next = NULL;
        task->files.fileList = freeHandle;
    }
    else
    {
        freeHandle = task->files.fileList;
        while(freeHandle->next)
        {
            if(freeHandle->free)
            {
                *handleNumber = freeHandle->type.freeHandleRange.first;
                break;
            }
            freeHandle = freeHandle->next;
        }
    }
    
    if(0 == *handleNumber)
    {
        *handleNumber = -1;
        MmFreeKernelHeap(handle);
        return OUT_OF_RESOURCES;
    }

    handle->free = false;
    handle->type.fileHandle.offset = 0;
    handle->type.fileHandle.id = *handleNumber;
    handle->type.fileHandle.flags = flags;

    freeHandle->type.freeHandleRange.first++;
    //free handle range exhausted
    if(freeHandle->type.freeHandleRange.first > freeHandle->type.freeHandleRange.last)
    {
        if(freeHandle->previous)
            freeHandle->previous->next = handle;
        else
            task->files.fileList = handle;
        if(freeHandle->next)
            freeHandle->next->previous = handle;
        handle->previous = freeHandle->previous;
        handle->next = freeHandle->next;
        MmFreeKernelHeap(freeHandle);
    }
    else
    {
        handle->next = freeHandle->next;
        handle->previous = freeHandle;
        if(handle->next)
            handle->next->previous = handle;
        freeHandle->next = handle;
    }
    
    handle->type.fileHandle.node->referenceCount++;
    task->files.openFileCount++;
    //TODO: update last use and flags

    return OK;
}

STATUS IoOpenKernelFile(char *file, IoFileOpenMode mode, IoFileFlags flags, struct IoFileHandle **handle)
{   
    ASSERT(file && handle);

    STATUS ret = OK;
    if(OK != (ret = open(file, mode, flags, handle)))
        return ret;

    (*handle)->free = false;
    (*handle)->type.fileHandle.offset = 0;
    (*handle)->type.fileHandle.id = 0;
    
    return OK;
}

STATUS IoCloseFile(struct KeTaskControlBlock *task, int handleNumber)
{
    ASSERT(task);
    if(task->parent)
        task = task->parent;

    if(NULL == task->files.fileList)
        return IO_FILE_NOT_FOUND;
    
    if(handleNumber < 1)
        return IO_FILE_NOT_FOUND;
    
    struct IoFileHandle *handle = task->files.fileList;
    while(handle)
    {
        if((false == handle->free) && (handle->type.fileHandle.id == handleNumber))
        {
            break;
        }
        handle = handle->next;
    }
    if(NULL == handle)
    {
        return IO_FILE_NOT_FOUND;
    }

    bool foundFreeEntry = false;
    struct IoFileHandle *freeHandle = task->files.fileList;
    while(freeHandle->next)
    {
        if(true == freeHandle->free)
        {
            if(freeHandle->type.freeHandleRange.first == (handleNumber + 1))
            {
                freeHandle->type.freeHandleRange.first--;
                foundFreeEntry = true;
                break;
            }
            else if(freeHandle->type.freeHandleRange.last == (handleNumber - 1))
            {
                freeHandle->type.freeHandleRange.last++;
                foundFreeEntry = true;
                break;
            }
        }
        freeHandle = freeHandle->next;
    }

    if(handle->type.fileHandle.node->referenceCount)
        handle->type.fileHandle.node->referenceCount--;

    if(false == foundFreeEntry) //free range entry not found, reuse file handle then
    {
        handle->free = true;
        handle->type.freeHandleRange.first = handleNumber;
        handle->type.freeHandleRange.last = handleNumber;
    }
    else
    {
        if(handle->previous)
            handle->previous->next = handle->next;
        else
            task->files.fileList = handle->next;
        
        if(handle->next)
            handle->next->previous = handle->previous;
        
        MmFreeKernelHeap(handle);
    }
    
    //TODO: flags, unlock file if locked by us
    return OK;
}

STATUS IoCloseKernelFile(struct IoFileHandle *handle)
{
    if(NULL == handle)
        return IO_FILE_NOT_FOUND;
    
    if(handle->type.fileHandle.node->referenceCount)
        handle->type.fileHandle.node->referenceCount--;

    MmFreeKernelHeap(handle);
    
    return OK;
}

static STATUS read(struct IoFileHandle *handle, void *buffer, uint64_t size, uint64_t offset, uint64_t *actualSize)
{
    ASSERT(buffer && actualSize);

    if(NULL == handle)
        return IO_FILE_NOT_FOUND;
    
    return IoVfsRead(handle->type.fileHandle.node, handle->type.fileHandle.flags, buffer, size, offset, actualSize);
}

STATUS IoReadFile(struct KeTaskControlBlock *task, int handle, void *buffer, uint64_t size, uint64_t offset, uint64_t *actualSize)
{
    ASSERT(task && buffer && actualSize);
    if(task->parent)
        task = task->parent;
    
    struct IoFileHandle *h = task->files.fileList;
    while(h)
    {
        if(h->type.fileHandle.id == handle)
            break;
        h = h->next;
    }
    if(NULL == h)
        return IO_FILE_NOT_FOUND;
    
    return read(h, buffer, size, offset, actualSize);
}

STATUS IoReadKernelFile(struct IoFileHandle *handle, void *buffer, uint64_t size, uint64_t offset, uint64_t *actualSize)
{
    ASSERT(buffer && actualSize);
    return read(handle, buffer, size, offset, actualSize);
}

static STATUS write(struct IoFileHandle *handle, void *buffer, uint64_t size, uint64_t offset, uint64_t *actualSize)
{
    ASSERT(buffer && actualSize);

    if(NULL == handle)
        return IO_FILE_NOT_FOUND;
    
    return IoVfsWrite(handle->type.fileHandle.node, handle->type.fileHandle.flags, buffer, size, offset, actualSize);
}

STATUS IoWriteFile(struct KeTaskControlBlock *task, int handle, void *buffer, uint64_t size, uint64_t offset, uint64_t *actualSize)
{
    ASSERT(task && buffer && actualSize);
    if(task->parent)
        task = task->parent;
    
    struct IoFileHandle *h = task->files.fileList;
    while(h)
    {
        if(h->type.fileHandle.id == handle)
            break;
        h = h->next;
    }
    if(NULL == h)
        return IO_FILE_NOT_FOUND;
    
    return write(h, buffer, size, offset, actualSize);
}

STATUS IoWriteKernelFile(struct IoFileHandle *handle, void *buffer, uint64_t size, uint64_t offset, uint64_t *actualSize)
{
    ASSERT(buffer && actualSize);
    return write(handle, buffer, size, offset, actualSize);
}

STATUS IoFsInit(void)
{
    return OK;
}

bool IoCheckIfFileExists(char *file)
{
    return (NULL != IoVfsGetNode(file));
}

STATUS IoGetFileSize(char *file, uint64_t *size)
{
    return IoVfsGetSize(file, size);
}