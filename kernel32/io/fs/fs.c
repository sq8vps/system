#include "fs.h"
#include "vfs.h"
#include "ke/task/task.h"
#include "ke/sched/sched.h"
#include "assert.h"
#include "rtl/string.h"
#include "mm/slab.h"
#include "mm/heap.h"

#define IO_FS_DEFAULT_FILE_HANDLE_LIMIT 1024
#define IO_FS_CHUNKS_PER_SLAB 16

#define IO_FS_HANDLE_BLOCK_SIZE 64 //number of file handles per one "file handle table block" allocation

struct
{
    void *slabHandle; /**< Slab allocator handle */
    uint32_t fileHandleLimit; /**< Per-process file handle count limit */
} static IoFsState = {.slabHandle = NULL, .fileHandleLimit = IO_FS_DEFAULT_FILE_HANDLE_LIMIT};

/**
 * @brief Open file and return kernel handle
 * @param *file File path
 * @param mode Open mode
 * @param flags Open flags
 * @param **handle Output file handle
 * @return Status code
 */
static STATUS IoOpenFileRaw(const char *file, IoFileOpenMode mode, IoFileFlags flags, struct IoFileHandle **handle);

/**
 * @brief Close file from kernel handle
 * @param *handle File handle
 * @return Status code
 */
static STATUS IoCloseFileRaw(struct IoFileHandle *handle);


/**
 * @brief Read/write file pseudo-synchronously
 * 
 * Perform read/write from/to file using internal callback, so that the task remains blocked until the operation is completed.
 * @param *tcb Target Task Control Block
 * @param handle File handle in given task
 * @param *buffer Source/target buffer
 * @param size Data size in bytes
 * @param offest Offset within the file
 * @param *actualSize Output number of bytes actually read/written
 * @param write True to write, false to read
 * @return Status code
 */
static STATUS IoReadWriteFileSync(struct KeTaskControlBlock *tcb, int handle, void *buffer, size_t size, uint64_t offset, size_t *actualSize, bool write);

/**
 * @brief Callback on read/write completion
 * @param status Operation status
 * @param actualSize Number of bytes actually processed
 * @param *context Context provided when calling \a IoReadWriteFileSync()
 */
static void IoFileReadWriteCallback(STATUS status, size_t actualSize, void *context)
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

STATUS IoOpenFile(const char *file, IoFileOpenMode mode, IoFileFlags flags, int *handleNumber)
{
    ASSERT(file && handleNumber);

    STATUS status = OK;
    struct IoFileHandle *handle = NULL;
    struct KeProcessControlBlock *pcb = KeGetCurrentTaskParent();
    *handleNumber = -1;
    
    handle = MmSlabAllocate(IoFsState.slabHandle);
    if(NULL == handle)
    {
        return OUT_OF_RESOURCES;
    }

    RtlMemset(handle, 0, sizeof(*handle));
    ObInitializeObjectHeader(handle);

    status = IoOpenFileRaw(file, mode, flags, &handle);
    if(OK != status)
    {
        MmSlabFree(IoFsState.slabHandle, handle);
        return status;
    }

    handle->id = -1;

    PRIO prio = ObLockObject(pcb);

    if(pcb->files.count >= IoFsState.fileHandleLimit)
    {
        ObUnlockObject(pcb, prio);
        IoCloseFileRaw(handle);
        return OUT_OF_RESOURCES;
    }

    if(pcb->files.count < pcb->files.tableSize)
    {
        for(uint32_t i = 0; i < pcb->files.tableSize; i++)
        {
            if(NULL == pcb->files.table[i].handle)
            {
                handle->id = i;
                break;
            }
        }
    }

    if(-1 == handle->id)
    {
        void *p = MmReallocateKernelHeap(pcb->files.table, sizeof(*pcb->files.table) * (pcb->files.tableSize + IO_FS_HANDLE_BLOCK_SIZE));
        if(NULL != p)
        {
            pcb->files.table = p;
            RtlMemset(&pcb->files.table[pcb->files.tableSize], 0, sizeof(*pcb->files.table) * IO_FS_HANDLE_BLOCK_SIZE);
            handle->id = pcb->files.tableSize;
            pcb->files.tableSize += IO_FS_HANDLE_BLOCK_SIZE;
        }
        else
        {
            ObUnlockObject(pcb, prio);
            IoCloseFileRaw(handle);
            return OUT_OF_RESOURCES;
        }
    }

    pcb->files.table[handle->id].handle = handle;
    pcb->files.count++;
    barrier();
    ObUnlockObject(pcb, prio);

    *handleNumber = handle->id;

    //TODO: update last use and flags

    return status;
}

static STATUS IoOpenFileRaw(const char *file, IoFileOpenMode mode, IoFileFlags flags, struct IoFileHandle **handle)
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
                if(NULL == *handle)
                {
                    mustClose = true;
                    status = OUT_OF_RESOURCES;
                }
                else
                {
                    RtlMemset(*handle, 0, sizeof(**handle));
                    ObInitializeObjectHeader(*handle);
                }
            }
                
            if(NULL != *handle)
            {
                (*handle)->node = fileNode;
                (*handle)->mode = mode;
                (*handle)->flags = flags;
                (*handle)->references = 1;
            }
        }
    }
    IoVfsUnlockTree();
    if(mustClose)
        IoVfsClose(fileNode);

    return status;
}

STATUS IoCloseFile(int handleNumber)
{
    if(handleNumber < 0)
        return FILE_NOT_FOUND;

    struct KeProcessControlBlock *pcb = KeGetCurrentTaskParent();

    STATUS status = OK;
    PRIO prio;
    struct IoFileHandle *handle = NULL;

    prio = ObLockObject(pcb);
    if(handleNumber > pcb->files.tableSize)
    {
        ObUnlockObject(pcb, prio);
        return FILE_NOT_FOUND;
    }
    ObUnlockObject(pcb, prio);

    KeAcquireMutex(&pcb->files.table[handleNumber].mutex);

    handle = pcb->files.table[handleNumber].handle;

    if(NULL == handle)
    {
        KeReleaseMutex(&pcb->files.table[handleNumber].mutex);
        return FILE_NOT_FOUND; 
    }
    
    prio = ObLockObject(pcb);
    pcb->files.table[handleNumber].handle = NULL;
    pcb->files.count--;
    barrier();
    ObUnlockObject(pcb, prio);

    KeReleaseMutex(&pcb->files.table[handleNumber].mutex);

    if(0 == __atomic_sub_fetch(&(handle->references), 1, __ATOMIC_RELAXED))
    {
        status = IoVfsClose(handle->node);

        MmSlabFree(IoFsState.slabHandle, handle);
    }

    //TODO: flags
    return status;
}

static STATUS IoCloseFileRaw(struct IoFileHandle *handle)
{
    STATUS status = OK;

    if(NULL == handle)
        return FILE_NOT_FOUND;
    
    if(0 == __atomic_sub_fetch(&(handle->references), 1, __ATOMIC_RELAXED))
    {
        status = IoVfsClose(handle->node);

        MmSlabFree(IoFsState.slabHandle, handle);
    }
    
    return status;
}

STATUS IoReadFile(int handle, void *buffer, size_t size, uint64_t offset, 
    IoReadWriteCompletionCallback callback, void *context)
{
    if(handle < 0)
        return FILE_NOT_FOUND;

    ASSERT(buffer);
    STATUS status = OK;
    struct IoFileHandle *h = NULL;
    
    struct KeProcessControlBlock *pcb = KeGetCurrentTaskParent();
    PRIO prio = ObLockObject(pcb);
    if(handle > pcb->files.tableSize)
    {
        ObUnlockObject(pcb, prio);
        return FILE_NOT_FOUND;
    }
    ObUnlockObject(pcb, prio);

    KeAcquireMutex(&pcb->files.table[handle].mutex);

    if(NULL == pcb->files.table[handle].handle)
    {
        KeReleaseMutex(&pcb->files.table[handle].mutex);
        return FILE_NOT_FOUND;
    }

    h = pcb->files.table[handle].handle;
    
    h->operation.write = 0;
    h->operation.offset = offset;
    status = IoVfsRead(h->node, h->flags, buffer, size, offset, callback, context);
    if(OK != status)
        KeReleaseMutex(&pcb->files.table[handle].mutex);
    
    return status;
}

STATUS IoWriteFile(int handle, void *buffer, size_t size, uint64_t offset,
    IoReadWriteCompletionCallback callback, void *context)
{
    if(handle < 0)
        return FILE_NOT_FOUND;

    ASSERT(buffer);
    STATUS status = OK;
    
    struct KeProcessControlBlock *pcb = KeGetCurrentTaskParent();
    PRIO prio = ObLockObject(pcb);
    if(handle > pcb->files.tableSize)
    {
        ObUnlockObject(pcb, prio);
        return FILE_NOT_FOUND;
    }
    ObUnlockObject(pcb, prio);

    KeAcquireMutex(&pcb->files.table[handle].mutex);

    if(NULL == pcb->files.table[handle].handle)
    {
        KeReleaseMutex(&pcb->files.table[handle].mutex);
        return FILE_NOT_FOUND;
    }

    struct IoFileHandle *h = pcb->files.table[handle].handle;
    
    if(h->mode & IO_FILE_APPEND)
        offset = h->node->size;

    h->operation.write = 1;
    h->operation.offset = offset;
    status = IoVfsWrite(h->node, h->flags, buffer, size, offset, callback, context);
    if(OK != status)
        KeReleaseMutex(&pcb->files.table[handle].mutex);
    
    return status;
}

static STATUS IoReadWriteFileSync(struct KeTaskControlBlock *tcb, int handle, void *buffer, size_t size, uint64_t offset, size_t *actualSize, bool write)
{
    ASSERT(tcb && buffer);
    
    struct KeProcessControlBlock *pcb = tcb->parent;
    PRIO prio = ObLockObject(pcb);
    if(handle > pcb->files.tableSize)
    {
        ObUnlockObject(pcb, prio);
        return FILE_NOT_FOUND;
    }
    ObUnlockObject(pcb, prio);

    KeAcquireMutex(&pcb->files.table[handle].mutex);

    if(NULL == pcb->files.table[handle].handle)
    {
        KeReleaseMutex(&pcb->files.table[handle].mutex);
        return FILE_NOT_FOUND;
    }

    struct IoFileHandle *h = pcb->files.table[handle].handle;

    prio = ObLockObject(h);
    h->operation.task = tcb;
    h->operation.completed = 0;
    ObUnlockObject(h, prio);

    STATUS status = OK;
    if(!write)
    {
        h->operation.write = 0;
        h->operation.offset = offset;
        status = IoVfsRead(h->node, h->flags, buffer, size, offset, IoFileReadWriteCallback, h);
    }
    else
    {
        if(h->mode & IO_FILE_APPEND)
            offset = h->node->size;

        h->operation.write = 1;
        h->operation.offset = offset;
        status = IoVfsWrite(h->node, h->flags, buffer, size, offset, IoFileReadWriteCallback, h);
    }

    if(OK != status)
    {
        KeReleaseMutex(&pcb->files.table[handle].mutex);
        return status;
    }
    
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

    if(NULL != actualSize) 
        *actualSize = h->operation.actualSize;
    status = h->operation.status;

    barrier();
    KeReleaseMutex(&pcb->files.table[handle].mutex);

    return status;
}

STATUS IoReadFileSync(int handle, void *buffer, size_t size, uint64_t offset, size_t *actualSize)
{
    return IoReadWriteFileSync(KeGetCurrentTask(), handle, buffer, size, offset, actualSize, false);
}

STATUS IoWriteFileSync(int handle, void *buffer, size_t size, uint64_t offset, size_t *actualSize)
{
    return IoReadWriteFileSync(KeGetCurrentTask(), handle, buffer, size, offset, actualSize, true);
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