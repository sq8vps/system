#include "fs.h"
#include "vfs.h"
#include "ke/task/task.h"
#include "ke/sched/sched.h"
#include "assert.h"
#include "rtl/string.h"
#include "mm/slab.h"

#define IO_FS_DEFAULT_FILE_HANDLE_LIMIT 1024
#define IO_FS_CHUNKS_PER_SLAB 16

struct
{
    void *slabHandle; /**< Slab allocator handle */
    uint32_t fileHandleLimit; /**< Per-process file handle count limit */
} static IoFsState = {.slabHandle = NULL, .fileHandleLimit = IO_FS_DEFAULT_FILE_HANDLE_LIMIT};

static void IoFileReadWriteCallback(STATUS status, uint64_t actualSize, void *context)
{
    struct IoFileHandle *h = context;
    if(NULL != h)
    {
        PRIO prio = ObLockObject(h);
        h->operation.actualSize = actualSize;
        h->operation.status = status;
        if(OK == status)
        {
            if(h->operation.write)
            {
                //check if file size changed
                if((h->operation.offset + actualSize) != h->type.fileHandle.node->size)
                {
                    h->type.fileHandle.node->size = h->operation.offset + actualSize;
                    h->type.fileHandle.node->flags |= IO_VFS_FLAG_ATTRIBUTES_DIRTY;
                }
            }
        }
        
        h->operation.completed = 1;
        KeUnblockTask(h->task);
        ObUnlockObject(h, prio);
    }
}

STATUS IoOpenFile(const char *file, IoFileOpenMode mode, IoFileFlags flags, const struct KeTaskControlBlock *tcb, int *handleNumber)
{
    // ASSERT(file && handleNumber && tcb);
    // struct KeProcessControlBlock *pcb = tcb->parent;
    // PRIO prio = ObLockObject(pcb);

    // if(pcb->files.count > IoFsState.fileHandleLimit)
    // {
    //     ObUnlockObject(pcb, prio);
    //     *handleNumber = -1;
    //     return FILE_COUNT_LIMIT_REACHED;
    // }

    // struct IoFileHandle *handle = NULL;
    // STATUS ret = OK;
    // if(OK != (ret = IoOpenKernelFile(file, mode, flags, &handle)))
    //     return ret;

    // //handle for free handle range entry
    // struct IoFileHandle *freeHandle;
    // if(NULL == pcb->files.list)
    // {
    //     freeHandle = MmSlabAllocate(IoFsState.slabHandle);
    //     if(NULL == freeHandle)
    //     {
    //         *handleNumber = -1;
    //         MmSlabFree(IoFsState.slabHandle, handle);
    //         return OUT_OF_RESOURCES;
    //     }
    //     ObInitializeObjectHeader(freeHandle);
    //     freeHandle->free = true;
    //     freeHandle->previous = NULL;
    //     freeHandle->type.freeHandleRange.first = 1;
    //     freeHandle->type.freeHandleRange.last = IoFsState.fileHandleLimit + 1;
    //     freeHandle->next = NULL;
    //     pcb->files.list = freeHandle;
    // }
    // else
    // {
    //     freeHandle = pcb->files.list;
    //     while(freeHandle->next)
    //     {
    //         if(freeHandle->free)
    //         {
    //             *handleNumber = freeHandle->type.freeHandleRange.first;
    //             break;
    //         }
    //         freeHandle = freeHandle->next;
    //     }
    // }
    
    // if(0 == *handleNumber)
    // {
    //     *handleNumber = -1;
    //     MmSlabFree(IoFsState.slabHandle, handle);
    //     return OUT_OF_RESOURCES;
    // }

    // handle->free = false;
    // handle->type.fileHandle.id = *handleNumber;
    // handle->type.fileHandle.flags = flags;

    // freeHandle->type.freeHandleRange.first++;
    // //free handle range exhausted
    // if(freeHandle->type.freeHandleRange.first > freeHandle->type.freeHandleRange.last)
    // {
    //     if(freeHandle->previous)
    //         freeHandle->previous->next = handle;
    //     else
    //         pcb->files.list = handle;
    //     if(freeHandle->next)
    //         freeHandle->next->previous = handle;
    //     handle->previous = freeHandle->previous;
    //     handle->next = freeHandle->next;
    //     MmSlabFree(IoFsState.slabHandle, freeHandle);
    // }
    // else
    // {
    //     handle->next = freeHandle->next;
    //     handle->previous = freeHandle;
    //     if(handle->next)
    //         handle->next->previous = handle;
    //     freeHandle->next = handle;
    // }
    
    // pcb->files.count++;
    // //TODO: update last use and flags

    return NOT_IMPLEMENTED;
}

STATUS IoOpenKernelFile(const char *file, IoFileOpenMode mode, IoFileFlags flags, struct IoFileHandle **handle)
{   
    ASSERT(file && handle);
    STATUS status = OK;
    bool mustClose = false;
    *handle = NULL;

    IoVfsLockTreeForWriting();
    struct IoVfsNode *fileNode = IoVfsGetNode(file);
    if(NULL == fileNode)
        status = FILE_NOT_FOUND;
    else if((IO_VFS_FILE != fileNode->type) && (IO_VFS_DEVICE != fileNode->type))
        status = BAD_FILE_TYPE;
    else if((mode & (IO_FILE_WRITE | IO_FILE_WRITE_ATTRIBUTES | IO_FILE_APPEND)) && (fileNode->flags & IO_VFS_FLAG_READ_ONLY))
        status = FILE_READ_ONLY;
    else
    {
        status = IoVfsOpen(fileNode, (mode & (IO_FILE_WRITE | IO_FILE_APPEND | IO_FILE_WRITE_ATTRIBUTES)) ? true : false,
                flags);
        
        if(OK == status)
        {
            //handle for newly opened file
            *handle = MmSlabAllocate(IoFsState.slabHandle);
            if(NULL == *handle)
            {
                mustClose = true;
                status = OUT_OF_RESOURCES;
            }
            else
            {
                RtlMemset(*handle, 0, sizeof(**handle));
                ObInitializeObjectHeader(*handle);

                (*handle)->type.fileHandle.node = fileNode;
                (*handle)->type.fileHandle.mode = mode;
                (*handle)->type.fileHandle.flags = flags;
                (*handle)->free = false;
                (*handle)->type.fileHandle.id = 0;
            }
        }
    }
    IoVfsUnlockTree();
    if(mustClose)
        IoVfsClose(fileNode);

    return status;
}

STATUS IoCloseFile(struct KeTaskControlBlock *tcb, int handleNumber)
{
    // ASSERT(tcb);
    // struct KeProcessControlBlock *pcb = tcb->parent;

    // STATUS status = OK;

    // if(NULL == pcb->files.list)
    //     return FILE_NOT_FOUND;
    
    // if(handleNumber < 1)
    //     return FILE_NOT_FOUND;
    
    // struct IoFileHandle *handle = pcb->files.list;
    // while(handle)
    // {
    //     if((false == handle->free) && (handle->type.fileHandle.id == handleNumber))
    //     {
    //         break;
    //     }
    //     handle = handle->next;
    // }
    // if(NULL == handle)
    // {
    //     return FILE_NOT_FOUND;
    // }

    // status = IoVfsClose(handle->type.fileHandle.node);

    // bool foundFreeEntry = false;
    // struct IoFileHandle *freeHandle = pcb->files.list;
    // while(freeHandle->next)
    // {
    //     if(true == freeHandle->free)
    //     {
    //         if(freeHandle->type.freeHandleRange.first == (handleNumber + 1))
    //         {
    //             freeHandle->type.freeHandleRange.first--;
    //             foundFreeEntry = true;
    //             break;
    //         }
    //         else if(freeHandle->type.freeHandleRange.last == (handleNumber - 1))
    //         {
    //             freeHandle->type.freeHandleRange.last++;
    //             foundFreeEntry = true;
    //             break;
    //         }
    //     }
    //     freeHandle = freeHandle->next;
    // }

    // if(false == foundFreeEntry) //free range entry not found, reuse file handle then
    // {
    //     handle->free = true;
    //     handle->type.freeHandleRange.first = handleNumber;
    //     handle->type.freeHandleRange.last = handleNumber;
    // }
    // else
    // {
    //     if(handle->previous)
    //         handle->previous->next = handle->next;
    //     else
    //         pcb->files.list = handle->next;
        
    //     if(handle->next)
    //         handle->next->previous = handle->previous;
        
    //     MmSlabFree(IoFsState.slabHandle, handle);
    // }

    // //TODO: flags
    // return status;
    return NOT_IMPLEMENTED;
}

STATUS IoCloseKernelFile(struct IoFileHandle *handle)
{
    if(NULL == handle)
        return FILE_NOT_FOUND;
    else if(handle->free)
        return FILE_NOT_FOUND;
    
    IoVfsClose(handle->type.fileHandle.node);

    MmSlabFree(IoFsState.slabHandle, handle);
    
    return OK;
}

STATUS IoReadFile(struct KeTaskControlBlock *tcb, int handle, void *buffer, uint64_t size, uint64_t offset, 
    IoReadWriteCompletionCallback callback, void *context)
{
    ASSERT(tcb && buffer);
    
    struct IoFileHandle *h = tcb->parent->files.list;
    while(h)
    {
        if(h->type.fileHandle.id == handle)
            break;
        h = h->next;
    }
    
    return IoReadKernelFile(h, buffer, size, offset, callback, context);
}

STATUS IoReadKernelFile(struct IoFileHandle *handle, void *buffer, uint64_t size, uint64_t offset,
    IoReadWriteCompletionCallback callback, void *context)
{
    ASSERT(buffer);
    if(NULL == handle)
        return FILE_NOT_FOUND;
    if(handle->free)
        return FILE_NOT_FOUND;
    
    handle->operation.write = 0;
    handle->operation.offset = offset;

    return IoVfsRead(handle->type.fileHandle.node, handle->type.fileHandle.flags, buffer, size, offset, callback, context);
}

STATUS IoWriteFile(struct KeTaskControlBlock *tcb, int handle, void *buffer, uint64_t size, uint64_t offset,
    IoReadWriteCompletionCallback callback, void *context)
{
    ASSERT(tcb && buffer);
    
    
    struct IoFileHandle *h = tcb->parent->files.list;
    while(h)
    {
        if(h->type.fileHandle.id == handle)
            break;
        h = h->next;
    }
    
    return IoWriteKernelFile(h, buffer, size, offset, callback, context);
}

STATUS IoWriteKernelFile(struct IoFileHandle *handle, void *buffer, uint64_t size, uint64_t offset, 
    IoReadWriteCompletionCallback callback, void *context)
{
    ASSERT(buffer);
    if(NULL == handle)
        return FILE_NOT_FOUND;
    if(handle->free)
        return FILE_NOT_FOUND;
    if(0 == (handle->type.fileHandle.mode & (IO_FILE_WRITE | IO_FILE_APPEND)))
        return FILE_BAD_MODE;
    if(handle->type.fileHandle.mode & IO_FILE_APPEND)
        offset = handle->type.fileHandle.node->size;

    handle->operation.write = 1;
    handle->operation.offset = offset;

    return IoVfsWrite(handle->type.fileHandle.node, handle->type.fileHandle.flags, buffer, size, offset, callback, context);
}

STATUS IoReadWriteKernelFileSync(struct IoFileHandle *handle, void *buffer, uint64_t size, uint64_t offset, uint64_t *actualSize, bool write)
{
    struct KeTaskControlBlock *task = KeGetCurrentTask();
    
    PRIO prio = ObLockObject(handle);
    handle->task = task;
    handle->operation.completed = 0;
    ObUnlockObject(handle, prio);

    STATUS status = OK;
    if(!write)
        IoReadKernelFile(handle, buffer, size, offset, IoFileReadWriteCallback, handle);
    else
        IoWriteKernelFile(handle, buffer, size, offset, IoFileReadWriteCallback, handle);

    if(OK != status)
        return status;
    
    prio = ObLockObject(handle);
    if(!handle->operation.completed)
    {
        //operation waiting to be completed
        KeBlockTask(task, TASK_BLOCK_IO);
        ObUnlockObject(handle, prio);
        
        KeTaskYield();
    }
    else
        ObUnlockObject(handle, prio);

    *actualSize = handle->operation.actualSize;
    return handle->operation.status;
}

STATUS IoReadWriteFileSync(struct KeTaskControlBlock *tcb, int handle, void *buffer, uint64_t size, uint64_t offset, uint64_t *actualSize, bool write)
{
    ASSERT(tcb && buffer);
    
    struct IoFileHandle *h = tcb->parent->files.list;
    while(h)
    {
        if(h->type.fileHandle.id == handle)
            break;
        h = h->next;
    }

    if(NULL == h)
        return FILE_NOT_FOUND;

    PRIO prio = ObLockObject(h);
    h->task = tcb;
    h->operation.completed = 0;
    ObUnlockObject(h, prio);
    

    STATUS status = OK;
    if(!write)
        IoReadKernelFile(h, buffer, size, offset, IoFileReadWriteCallback, h);
    else
        IoWriteKernelFile(h, buffer, size, offset, IoFileReadWriteCallback, h);

    if(OK != status)
        return status;
    
    prio = ObLockObject(h);
    if(!h->operation.completed)
    {
        //operation waiting to be completed
        KeBlockTask(tcb, TASK_BLOCK_IO);
        ObUnlockObject(h, prio);
        
        KeTaskYield();
    }
    else
        ObUnlockObject(h, prio);
        

    *actualSize = h->operation.actualSize;
    return h->operation.status;
}

STATUS IoReadKernelFileSync(struct IoFileHandle *handle, void *buffer, uint64_t size, uint64_t offset, uint64_t *actualSize)
{
    return IoReadWriteKernelFileSync(handle, buffer, size, offset, actualSize, false);
}

STATUS IoWriteKernelFileSync(struct IoFileHandle *handle, void *buffer, uint64_t size, uint64_t offset, uint64_t *actualSize)
{
    return IoReadWriteKernelFileSync(handle, buffer, size, offset, actualSize, true);
}

STATUS IoReadFileSync(struct KeTaskControlBlock *task, int handle, void *buffer, uint64_t size, uint64_t offset, uint64_t *actualSize)
{
    return IoReadWriteFileSync(task, handle, buffer, size, offset, actualSize, false);
}

STATUS IoWriteFileSync(struct KeTaskControlBlock *task, int handle, void *buffer, uint64_t size, uint64_t offset, uint64_t *actualSize)
{
    return IoReadWriteFileSync(task, handle, buffer, size, offset, actualSize, true);
}

STATUS IoFsInit(void)
{
    IoFsState.slabHandle = MmSlabCreate(sizeof(struct IoFileHandle), IO_FS_CHUNKS_PER_SLAB);
    if(NULL != IoFsState.slabHandle)
        return OK;
    else
        return VFS_INITIALIZATION_FAILED;
}

bool IoCheckIfFileExists(const char *file)
{
    IoVfsLockTreeForWriting();
    bool status = (NULL != IoVfsGetNode(file));
    IoVfsUnlockTree();
    return status;
}

STATUS IoGetFileSize(const char *file, uint64_t *size)
{
    return IoVfsGetSize(file, size);
}