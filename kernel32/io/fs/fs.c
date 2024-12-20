#include "fs.h"
#include "vfs.h"
#include "ke/task/task.h"
#include "ke/sched/sched.h"
#include "assert.h"
#include "rtl/string.h"
#include "mm/heap.h"

#define IO_FS_DEFAULT_FILE_HANDLE_LIMIT 1024

#define IO_FS_HANDLE_BLOCK_SIZE 64 //number of file handles per one "file handle table block" allocation

static struct
{
    uint32_t fileHandleLimit; /**< Per-process file handle count limit */
} IoFsState = {.fileHandleLimit = IO_FS_DEFAULT_FILE_HANDLE_LIMIT};

/**
 * @brief Open file and return kernel handle
 * @param *file File path if \a fileNode not provided
 * @param *fileNode VFS file node, used instead of \a file if not NULL
 * @param *taskfs Task file system context, must be provided if \a fileNode is not NULL
 * @param mode Open mode
 * @param flags Open flags
 * @param **handle Output file handle
 * @return Status code
 */
static STATUS IoOpenFileRaw(const char *file, struct IoVfsNode *fileNode, struct IoTaskFsContext *taskfs, IoFileOpenMode mode, IoFileFlags flags, struct IoFileHandle **handle);

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
        PRIO prio = KeAcquireSpinlock(&(h->operation.lock));
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
        KeReleaseSpinlock(&(h->operation.lock), prio);
    }
}

STATUS IoOpenFileForProcess(struct KeProcessControlBlock *pcb, const char *file, struct IoVfsNode *fileNode, struct IoTaskFsContext *taskfs, IoFileOpenMode mode, IoFileFlags flags, int *handleNumber)
{
    ASSERT((pcb || file) && handleNumber);

    STATUS status = OK;
    struct IoFileHandle *handle = NULL;

    if((flags & IO_FILE_FLAG_FORCE_HANDLE_NUMBER) && ((uint32_t)(*handleNumber) >= IoFsState.fileHandleLimit))
        return OUT_OF_RESOURCES;

    status = IoOpenFileRaw(file, fileNode, taskfs, mode, flags, &handle);
    if(OK != status)
    {
        return status;
    }

    handle->id = -1;

    ObLockObject(pcb);

    if(pcb->files.count >= IoFsState.fileHandleLimit)
    {
        ObUnlockObject(pcb);
        IoCloseFileRaw(handle);
        return OUT_OF_RESOURCES;
    }

    if((flags & IO_FILE_FLAG_FORCE_HANDLE_NUMBER) && (*handleNumber >= 0))
    {
        if(pcb->files.tableSize > (uint32_t)(*handleNumber))
        {
            if(NULL == pcb->files.table[*handleNumber].handle)
            {
                handle->id = *handleNumber;
            }
            else
            {
                ObUnlockObject(pcb);
                IoCloseFileRaw(handle);
                return FILE_ALREADY_EXISTS;
            }
        }
        else
        {
            void *p = MmReallocateKernelHeap(pcb->files.table, sizeof(*pcb->files.table) * ALIGN_UP(*handleNumber + 1, IO_FS_HANDLE_BLOCK_SIZE));
            if(NULL != p)
            {
                pcb->files.table = p;
                RtlMemset(&pcb->files.table[pcb->files.tableSize], 0, sizeof(*pcb->files.table) * (ALIGN_UP(*handleNumber + 1, IO_FS_HANDLE_BLOCK_SIZE) - pcb->files.tableSize));
                handle->id = *handleNumber;
                pcb->files.tableSize = ALIGN_UP(*handleNumber + 1, IO_FS_HANDLE_BLOCK_SIZE);
            }
            else
            {
                ObUnlockObject(pcb);
                IoCloseFileRaw(handle);
                return OUT_OF_RESOURCES;
            } 
        }
    }
    else
    {
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
                ObUnlockObject(pcb);
                IoCloseFileRaw(handle);
                return OUT_OF_RESOURCES;
            }
        }
    }

    pcb->files.table[handle->id].handle = handle;
    pcb->files.count++;
    barrier();
    ObUnlockObject(pcb);

    *handleNumber = handle->id;

    //TODO: update last use and flags

    return status;
}

STATUS IoOpenFile(const char *file, IoFileOpenMode mode, IoFileFlags flags, int *handleNumber)
{
    return IoOpenFileForProcess(KeGetCurrentTaskParent(), file, NULL, NULL, mode, flags, handleNumber);
}

static STATUS IoOpenFileRaw(const char *file, struct IoVfsNode *fileNode, struct IoTaskFsContext *taskfs, IoFileOpenMode mode, IoFileFlags flags, struct IoFileHandle **handle)
{   
    ASSERT((file || fileNode) && handle);
    STATUS status = OK;
    bool mustClose = false;
    *handle = NULL;
    struct IoTaskFsContext taskfsBuffer = IO_TASK_FS_CONTEXT_INITIALIZER;

    IoVfsLockTreeForWriting();
    if(NULL == taskfs)
        taskfs = &taskfsBuffer;

    if(NULL == fileNode)
    {
        taskfs = &taskfsBuffer;
        fileNode = IoVfsGetNode(file, taskfs);
        if(!(flags & IO_FILE_NO_LINK_RESOLUTION))
            fileNode = IoVfsResolveLink(fileNode, taskfs);
    }

    if(NULL == fileNode)
        status = FILE_NOT_FOUND;
    else if((IO_VFS_FILE != fileNode->type) && (IO_VFS_DEVICE != fileNode->type) && (IO_VFS_LINK != fileNode->type))
        status = BAD_FILE_TYPE;
    else if((mode & (IO_FILE_WRITE | IO_FILE_WRITE_ATTRIBUTES | IO_FILE_APPEND)) && (fileNode->flags & IO_VFS_FLAG_READ_ONLY))
        status = FILE_READ_ONLY;
    else
    {
        status = IoVfsOpen(fileNode, (mode & (IO_FILE_WRITE | IO_FILE_APPEND | IO_FILE_WRITE_ATTRIBUTES)) ? true : false,
                flags);
        
        if(OK == status)
        {
            *handle = ObCreateObject(OB_FILE);
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
                (*handle)->references = 1;
                (*handle)->operation.lock = (KeSpinlock)KeSpinlockInitializer;
                (*handle)->taskfs = *taskfs;
            }
        }
    }
    IoVfsUnlockTree();
    if(mustClose)
        IoVfsClose(fileNode);

    return status;
}

STATUS IoCloseFileForProcess(struct KeProcessControlBlock *pcb, int handleNumber)
{
    if(handleNumber < 0)
        return FILE_NOT_FOUND;

    STATUS status = OK;
    struct IoFileHandle *handle = NULL;

    if(handleNumber < 0)
        return FILE_NOT_FOUND;

    ObLockObject(pcb);
    if((uint32_t)handleNumber >= pcb->files.tableSize)
    {
        ObUnlockObject(pcb);
        return FILE_NOT_FOUND;
    }
    ObUnlockObject(pcb);

    KeAcquireMutex(&pcb->files.table[handleNumber].mutex);

    handle = pcb->files.table[handleNumber].handle;

    if(NULL == handle)
    {
        KeReleaseMutex(&pcb->files.table[handleNumber].mutex);
        return FILE_NOT_FOUND; 
    }
    
    ObLockObject(pcb);
    pcb->files.table[handleNumber].handle = NULL;
    pcb->files.count--;
    barrier();
    ObUnlockObject(pcb);

    KeReleaseMutex(&pcb->files.table[handleNumber].mutex);

    if(0 == __atomic_sub_fetch(&(handle->references), 1, __ATOMIC_RELAXED))
    {
        status = IoVfsClose(handle->node);

        ObDestroyObject(handle);
    }

    //TODO: flags
    return status;
}

STATUS IoCloseFile(int handleNumber)
{
    return IoCloseFileForProcess(KeGetCurrentTaskParent(), handleNumber);
}

static STATUS IoCloseFileRaw(struct IoFileHandle *handle)
{
    STATUS status = OK;

    if(NULL == handle)
        return FILE_NOT_FOUND;
    
    if(0 == __atomic_sub_fetch(&(handle->references), 1, __ATOMIC_RELAXED))
    {
        status = IoVfsClose(handle->node);

        ObDestroyObject(handle);
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
    
    if(handle < 0)
        return FILE_NOT_FOUND;

    struct KeProcessControlBlock *pcb = KeGetCurrentTaskParent();
    ObLockObject(pcb);
    if((uint32_t)handle >= pcb->files.tableSize)
    {
        ObUnlockObject(pcb);
        return FILE_NOT_FOUND;
    }
    ObUnlockObject(pcb);

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
    ObLockObject(pcb);
    if((uint32_t)handle >= pcb->files.tableSize)
    {
        ObUnlockObject(pcb);
        return FILE_NOT_FOUND;
    }
    ObUnlockObject(pcb);

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

    if(handle < 0)
        return FILE_NOT_FOUND;
    
    struct KeProcessControlBlock *pcb = tcb->parent;
    ObLockObject(pcb);
    if((uint32_t)handle >= pcb->files.tableSize)
    {
        ObUnlockObject(pcb);
        return FILE_NOT_FOUND;
    }
    ObUnlockObject(pcb);

    KeAcquireMutex(&pcb->files.table[handle].mutex);

    if(NULL == pcb->files.table[handle].handle)
    {
        KeReleaseMutex(&pcb->files.table[handle].mutex);
        return FILE_NOT_FOUND;
    }

    struct IoFileHandle *h = pcb->files.table[handle].handle;

    PRIO prio = KeAcquireSpinlock(&(h->operation.lock));
    h->operation.task = tcb;
    h->operation.completed = 0;
    KeReleaseSpinlock(&(h->operation.lock), prio);

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
    
    prio = KeAcquireSpinlock(&(h->operation.lock));
    if(!h->operation.completed)
    {
        //operation waiting to be completed
        KeBlockTask(tcb, TASK_BLOCK_IO);
        KeReleaseSpinlock(&(h->operation.lock), prio);
        
        KeTaskYield();
    }
    else
        KeReleaseSpinlock(&(h->operation.lock), prio);

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
    return OK;
}

bool IoCheckIfFileExists(const char *file)
{
    IoVfsLockTreeForWriting();
    struct IoTaskFsContext taskfs = IO_TASK_FS_CONTEXT_INITIALIZER;
    bool status = (NULL != IoVfsGetNode(file, &taskfs));
    IoVfsUnlockTree();
    return status;
}

STATUS IoGetFileSize(const char *file, uint64_t *size)
{
    return IoVfsGetSize(file, size);
}

STATUS IoCloneFileToNewProcess(struct KeProcessControlBlock *pcb, int targetHandle, int sourceHandle)
{
    ASSERT(pcb);

    if((sourceHandle < 0) || (targetHandle < 0))
        return FILE_NOT_FOUND;

    STATUS status = OK;
    struct KeProcessControlBlock *spcb = KeGetCurrentTaskParent();

    ObLockObject(spcb);
    if((uint32_t)sourceHandle >= spcb->files.tableSize)
    {
        ObUnlockObject(spcb);
        return FILE_NOT_FOUND;
    }
    ObUnlockObject(spcb);

    KeAcquireMutex(&spcb->files.table[sourceHandle].mutex);

    if(NULL == spcb->files.table[sourceHandle].handle)
    {
        KeReleaseMutex(&spcb->files.table[sourceHandle].mutex);
        return FILE_NOT_FOUND;
    }

    struct IoFileHandle *h = spcb->files.table[sourceHandle].handle;
    status = IoOpenFileForProcess(pcb, NULL, h->node, &(h->taskfs), h->mode, h->flags | IO_FILE_FLAG_FORCE_HANDLE_NUMBER | IO_FILE_FLAG_NO_WAIT, &targetHandle);
    KeReleaseMutex(&spcb->files.table[sourceHandle].mutex);
    return status;
}

struct IoVfsNode* IoGetVfsNodeForFile(struct KeProcessControlBlock *pcb, int handle)
{
    struct IoVfsNode *n = NULL;
    if(handle < 0)
        return NULL;

    ObLockObject(pcb);
    if((uint32_t)handle >= pcb->files.tableSize)
    {
        ObUnlockObject(pcb);
        return NULL;
    }

    KeAcquireMutex(&(pcb->files.table[handle].mutex));
    if(NULL != pcb->files.table[handle].handle)
    {
        n = pcb->files.table[handle].handle->node;
    }
    
    KeReleaseMutex(&(pcb->files.table[handle].mutex));
    return n;
}