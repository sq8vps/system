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
                if((h->operation.offset + actualSize) != h->node->size)
                {
                    h->node->size = h->operation.offset + actualSize;
                    h->node->flags |= IO_VFS_FLAG_ATTRIBUTES_DIRTY;
                }
            }
        }
        
        h->operation.completed = 1;
        KeUnblockTask(h->operation.task);
        ObUnlockObject(h, prio);
    }
}

STATUS IoOpenFile(const char *file, IoFileOpenMode mode, IoFileFlags flags, const struct KeTaskControlBlock *tcb, int *handleNumber)
{
    ASSERT(file && handleNumber && tcb);

    STATUS status = OK;
    struct IoFileHandle *handle = NULL;
    struct KeProcessControlBlock *pcb = tcb->parent;
    *handleNumber = -1;

    PRIO prio = ObLockObject(pcb);

    if(pcb->files.count >= IoFsState.fileHandleLimit)
    {
        ObUnlockObject(pcb, prio);
        return OUT_OF_RESOURCES;
    }
    
    handle = MmSlabAllocate(IoFsState.slabHandle);
    if(NULL == handle)
    {
        ObUnlockObject(pcb, prio);
        return OUT_OF_RESOURCES;
    }

    RtlMemset(handle, 0, sizeof(*handle));
    ObInitializeObjectHeader(handle);

    struct IoFileHandle *h = pcb->files.list;
    if(NULL == h)
    {
        pcb->files.list = handle;
        handle->id = 0;
    }
    else
    {
        while(1)
        {
            if((NULL == h->next)
            || ((NULL != h->next) && ((h->next->id - h->id) > 1)))
            {
                handle->id = h->id + 1;
                if(NULL == h->next)
                {
                    handle->next = NULL;
                }
                else
                {
                    h->next->previous = handle;
                    handle->next = h->next;
                }
                handle->previous = h;
                h->next = handle;
                break;
            }
            h = h->next;
        }
    }

    pcb->files.count++;
    barrier();
    ObUnlockObject(pcb, prio);

    status = IoOpenKernelFile(file, mode, flags, &handle);
    if(OK != status)
    {
        prio = ObLockObject(pcb);
        if(NULL != handle->previous)
            handle->previous->next = handle->next;
        else
            pcb->files.list = handle->next;

        if(NULL != handle->next)
            handle->next->previous = handle->previous;
        
        pcb->files.count--;
        barrier();
        ObUnlockObject(pcb, prio);

        MmSlabFree(IoFsState.slabHandle, handle);
        return status;
    }

    *handleNumber = handle->id;

    //TODO: update last use and flags

    return status;
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
            if(NULL == *handle)
            {
                *handle = MmSlabAllocate(IoFsState.slabHandle);
                if(NULL != *handle)
                {
                    RtlMemset(*handle, 0, sizeof(**handle));
                    ObInitializeObjectHeader(*handle);
                }
            }

            if(NULL == *handle)
            {
                mustClose = true;
                status = OUT_OF_RESOURCES;
            }
            else
            {
                (*handle)->node = fileNode;
                (*handle)->mode = mode;
                (*handle)->flags = flags;
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
    ASSERT(tcb);
    struct KeProcessControlBlock *pcb = tcb->parent;

    STATUS status = OK;
    PRIO prio;
    struct IoFileHandle *handle = NULL;

    prio = ObLockObject(pcb);
    
    struct IoFileHandle *handle = pcb->files.list;
    while(NULL != handle)
    {
        if(handle->id == handleNumber)
        {
            break;
        }
        handle = handle->next;
    }

    if(NULL == handle)
    {
        ObUnlockObject(pcb, prio);
        return FILE_NOT_FOUND;
    }

    if(NULL != handle->previous)
        handle->previous->next = handle->next;
    else
        pcb->files.list = handle->next;

    if(NULL != handle->next)
        handle->next->previous = handle->previous;
    
    pcb->files.count--;
    barrier();
    ObUnlockObject(pcb, prio);

    status = IoVfsClose(handle->node);

    MmSlabFree(IoFsState.slabHandle, handle);

    //TODO: flags
    return status;
}

STATUS IoCloseKernelFile(struct IoFileHandle *handle)
{
    STATUS status = OK;

    if(NULL == handle)
        return FILE_NOT_FOUND;
    
    status = IoVfsClose(handle->node);

    MmSlabFree(IoFsState.slabHandle, handle);
    
    return status;
}

STATUS IoReadFile(struct KeTaskControlBlock *tcb, int handle, void *buffer, uint64_t size, uint64_t offset, 
    IoReadWriteCompletionCallback callback, void *context)
{
    ASSERT(tcb && buffer);
    
    struct IoFileHandle *h = tcb->parent->files.list;
    while(h)
    {
        if(h->id == handle)
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
    
    handle->operation.write = 0;
    handle->operation.offset = offset;

    return IoVfsRead(handle->node, handle->flags, buffer, size, offset, callback, context);
}

STATUS IoWriteFile(struct KeTaskControlBlock *tcb, int handle, void *buffer, uint64_t size, uint64_t offset,
    IoReadWriteCompletionCallback callback, void *context)
{
    ASSERT(tcb && buffer);
    
    
    struct IoFileHandle *h = tcb->parent->files.list;
    while(h)
    {
        if(h->id == handle)
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

    if(0 == (handle->mode & (IO_FILE_WRITE | IO_FILE_APPEND)))
        return FILE_BAD_MODE;
    if(handle->mode & IO_FILE_APPEND)
        offset = handle->node->size;

    handle->operation.write = 1;
    handle->operation.offset = offset;

    return IoVfsWrite(handle->node, handle->flags, buffer, size, offset, callback, context);
}

STATUS IoReadWriteKernelFileSync(struct IoFileHandle *handle, void *buffer, uint64_t size, uint64_t offset, uint64_t *actualSize, bool write)
{
    struct KeTaskControlBlock *task = KeGetCurrentTask();
    
    PRIO prio = ObLockObject(handle);
    handle->operation.task = task;
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
    
    struct KeProcessControlBlock *pcb = tcb->parent;
    PRIO prio;

    prio = ObLockObject(pcb);

    struct IoFileHandle *h = tcb->parent->files.list;
    while(NULL != h)
    {
        if(h->id == handle)
            break;
        h = h->next;
    }

    if(NULL == h)
    {
        ObUnlockObject(pcb, prio);
        return FILE_NOT_FOUND;
    }

    PRIO prio = ObLockObject(h);
    h->operation.task = tcb;
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