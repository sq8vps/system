#include "vfs.h"
#include "common.h"
#include "io/dev/dev.h"
#include "sdrv/initrd/initrd.h"
#include "assert.h"
#include "devfs.h"
#include "mm/slab.h"
#include "ke/core/mutex.h"
#include "io/dev/rp.h"
#include "ddk/fs.h"
#include "mm/heap.h"
#include "ke/task/task.h"
#include "ke/sched/sched.h"

#define IO_VFS_MAX_SYMLINK_DEPTH 10
#define IO_VFS_DEFAULT_MAX_FILE_NAME_LENGTH 127
#define IO_VFS_CHUNKS_PER_SLAB 16

static struct
{
    struct IoVfsNode *root; /**< Root node */
    void *slabHandle; /**< VFS node slab allocator handle */
    uint32_t maxFileNameLength; /**< Max file name length */
    KeSpinlock lock; /**< Global VFS tree spinlock */
} 
IoVfsState = {.root = NULL, .slabHandle = NULL, .maxFileNameLength = IO_VFS_DEFAULT_MAX_FILE_NAME_LENGTH, .lock = KeSpinlockInitializer};


struct IoVfsNode* IoVfsCreateNode(char *name)
{
    if(CmStrlen(name) > IoVfsState.maxFileNameLength)
        return NULL;
    
    if(!CmCheckFileName(name))
        return NULL;

    struct IoVfsNode *node = MmSlabAllocate(IoVfsState.slabHandle);
    if(NULL == node)
        return NULL;
    
    ObInitializeObjectHeader(node);
    
    CmStrncpy(node->name, name, IoVfsState.maxFileNameLength);

    return node;
}

uint32_t IoVfsGetMaxFileNameLength(void)
{
    return IoVfsState.maxFileNameLength;
}

void IoVfsDestroyNode(struct IoVfsNode *node)
{
    ASSERT(node);
    
    MmSlabFree(IoVfsState.slabHandle, node);
}

STATUS IoVfsInit(void)
{
    IoVfsState.slabHandle = MmSlabCreate(sizeof(struct IoVfsNode) + IoVfsState.maxFileNameLength + 1, IO_VFS_CHUNKS_PER_SLAB);
    if(NULL == IoVfsState.slabHandle)
        return OUT_OF_RESOURCES;

    IoVfsState.root = IoVfsCreateNode("");
    if(NULL == IoVfsState.root)
        return IO_VFS_INITIALIZATION_FAILED;

    //prepare root node
    IoVfsState.root->type = IO_VFS_DIRECTORY;
    IoVfsState.root->child = NULL;
    IoVfsState.root->flags = IO_VFS_FLAG_PERSISTENT;
    IoVfsState.root->fsType = IO_VFS_FS_VIRTUAL;

    return IoInitDeviceFs(IoVfsState.root);
}

char* IoVfsDetachFileName(char *path)
{
    ASSERT(path);
    char *c = path + CmStrlen(path);
    while(c >= path)
    {
        if('/' == *c)
        {
            *c = '\0';
            return c + 1;
        }
        c--;
    }
    //no slash was found, whole path is the file name
    return path;
}



STATUS IoVfsOpenNode(struct IoVfsNode *node, bool write, IoVfsFlags flags)
{
    ASSERT(node);
    STATUS status = OK;

    struct KeTaskControlBlock *task = KeGetCurrentTask();

    ObLockObject(node);
    //check if file is in use for writing
    if(NULL != node->currentTask)
    {
        if(flags & IO_VFS_FLAG_NO_WAIT)
        {
            ObUnlockObject(node);
            return IO_FILE_LOCKED;
        }

        struct KeTaskControlBlock *t = node->taskQueue;
        if(NULL == t)
        {
            node->taskQueue = task;
            KeBlockTask(task, TASK_BLOCK_IO);
            ObUnlockObject(node);
            KeTaskYield();
        }
        else
        {
            while(NULL != t)
            {
                ObLockObject(t);
                if(NULL == t->nextAux)
                {
                    t->nextAux = task;
                    task->previousAux = t;
                    KeBlockTask(task, TASK_BLOCK_IO);
                    ObUnlockObject(t);
                    ObUnlockObject(node);
                    KeTaskYield();
                    break;
                }
                struct KeTaskControlBlock *n = t->nextAux;
                ObUnlockObject(t);
                t = n;
            }
        }
    }
    if(write)
    {
        node->currentTask = task;
    }
    else
        node->references.readers++;

    ObUnlockObject(node);

    switch(node->fsType)
    {
        case IO_VFS_FS_PHYSICAL:
        case IO_VFS_FS_VIRTUAL:
            struct IoRp *rp = IoCreateRp();
            if(NULL != rp)
            {
                rp->code = IO_RP_OPEN;
                rp->vfsNode = node;
                status = IoSendRp(node->device, rp);
                if(OK == status)
                {
                    IoWaitForRpCompletion(rp);
                    status = rp->status;
                }
                IoFreeRp(rp);
            }
            else
                status = OUT_OF_RESOURCES;
            break;
        case IO_VFS_FS_TASKFS:
        case IO_VFS_FS_INITRD:
            break;
        default:
            status = IO_BAD_FILE_TYPE;
            break;
    }

    if(OK != status)
    {
        ObLockObject(node);
        if(write)
            node->currentTask = NULL;
        else
            node->references.readers--;
        
        //TODO: process queue
    }
    return status;
}

STATUS IoVfsClose(struct IoVfsNode *node)
{
    ASSERT(node);
    STATUS status = OK;

    switch(node->fsType)
    {
        case IO_VFS_FS_PHYSICAL:
        case IO_VFS_FS_VIRTUAL:
            struct IoRp *rp = IoCreateRp();
            if(NULL != rp)
            {
                rp->code = IO_RP_CLOSE;
                rp->vfsNode = node;
                status = IoSendRp(node->device, rp);
                if(OK == status)
                {
                    IoWaitForRpCompletion(rp);
                    status = rp->status;
                }
                IoFreeRp(rp);
            }
            else
                status = OUT_OF_RESOURCES;
            break;
        case IO_VFS_FS_TASKFS:
        case IO_VFS_FS_INITRD:
            break;
        default:
            status = IO_BAD_FILE_TYPE;
            break;
    }
    return status;
}

struct IoVfsNode *IoVfsResolveLink(struct IoVfsNode *node)
{
    ASSERT(node);

    if(IO_VFS_LINK != node->type)
        return node;
    
    uint32_t i = 0;
    while(i < IO_VFS_MAX_SYMLINK_DEPTH)
    {
        if(NULL != node->linkDestination)
        {
            //there is a cached link destination
            node = node->linkDestination;
            if(IO_VFS_LINK != node->type)
                return node;
        }
        else
        {
            //link destination needs to be obtained
            //TODO: read disk to find link destination
        }
    }
    return NULL;
}

struct IoVfsNode *IoVfsGetNode(struct IoVfsNode *parent, char *name)
{
    ASSERT(parent && name);
    STATUS status = OK;

    //"." file, which points to the same directory
    if(0 == CmStrcmp(name, "."))
        return parent;
    
    //".." file, which points to parent directory
    if(0 == CmStrcmp(name, ".."))
        return parent->parent;

    struct IoVfsNode *node = parent->child;

    //look for given child
    while(node)
    {
        if(0 == CmStrcmp(node->name, name))
            return node;
        
        node = node->next;
    }

    //child not found in cached tree, ask driver to find it on the physical storage
    uint64_t size = 0;
    union IoVfsReference ref; 
    switch(parent->fsType)
    {
        case IO_VFS_FS_PHYSICAL:
            struct IoRp *rp = IoCreateRp();
            if(NULL != rp)
            {
                union FsGetRequest *req = MmAllocateKernelHeap(sizeof(*req));
                if(NULL != req)
                {
                    req->getNode.name = name;
                    req->getNode.parent = parent;
                    rp->code = IO_RP_FILESYSTEM_CONTROL;
                    rp->payload.deviceControl.code = FS_GET_NODE;
                    rp->payload.deviceControl.data = req;

                    status = IoSendRp(parent->device, rp);
                    if(OK == status)
                    {
                        IoWaitForRpCompletion(rp);
                        status = rp->status;
                        if(OK == status)
                            node = req->getNode.node;
                        else
                            node = NULL;
                    }

                    MmFreeKernelHeap(req);
                }
                else
                    status = OUT_OF_RESOURCES;

                IoFreeRp(rp);
            }
            else 
                status = OUT_OF_RESOURCES;

            if(OK == status)
            {
                IoVfsInsertNode(node, parent);
                return node;
            }
            else
                return NULL;
            break;
        case IO_VFS_FS_INITRD:
            ref.u64 = InitrdGetFileReference(name, &size);
            if(0 != ref.u64)
            {
                node = IoVfsCreateNode(name);
                if(NULL == node)
                    return NULL;

                node->ref = ref;
                node->flags = IO_VFS_FLAG_NO_CACHE | IO_VFS_FLAG_READ_ONLY;
                node->fsType = IO_VFS_FS_INITRD;
                node->type = IO_VFS_FILE;
                node->size = size;
                IoVfsInsertNode(node, parent);
                return node;
            }
            break;
        case IO_VFS_FS_TASKFS:
            
            break;
    }


    return NULL;
}

struct IoVfsNode *IoVfsGetNodeByPath(char *path)
{
    struct IoVfsNode *node = IoVfsState.root;

    //treat empty string as root
    if('\0' == path[0])
        return IoVfsState.root;

    //every path must start from root
    if('/' != path[0])
        return NULL;
    
    //skip '/'
    path++;

    while('\0' != *path)
    {
        char *element = path;
        //find separator or end of path
        while(!(('/' == *path) || ('\0' == *path)))
        {
            path++;
        }
        //avoid altering the path
        //store original character and replace with null to tokenize
        char t = *path;
        *path = '\0';
        //get node
        node = IoVfsGetNode(node, element);
        //restore character
        *path = t; 
        //check if node was found
        if(NULL == node)
            return NULL;

        //check if this is the end of the path
        //treat ".../file/" the same as ".../file", doesn't matter if it's a directory or not
        if(('\0' == path[0])
            || (('/' == path[0]) && ('\0' == path[1])))
        {
            return node;
        }

        //go to the next character
        path++;

        //now check if given node is a symbolic link
        //if so, then resolve
        if(IO_VFS_LINK == node->type)
        {
            node = IoVfsResolveLink(node);
            if(NULL == node)
                return node;
        }
    }

    return node;
}

void IoVfsInsertNode(struct IoVfsNode *node, struct IoVfsNode *parent)
{
    ASSERT(node && parent);
    
    node->parent = parent;
    node->previous = NULL;

    //find next free position
    if(NULL != parent->child)
    {
        struct IoVfsNode *previous = parent->child;
        while(NULL != previous->next)
            previous = previous->next;
        
        previous->next = node;
        node->previous = previous;
    }
    else
        parent->child = node;

    node->next = NULL;
}

STATUS IoVfsInsertNodeByPath(struct IoVfsNode *node, char *path)
{
    ASSERT(node && path);
    struct IoVfsNode *target = IoVfsGetNodeByPath(path);
    if(NULL == target)
    {
        ERROR("Node %s not found\n", path);
        return IO_FILE_NOT_FOUND;
    }
    
    if((IO_VFS_DIRECTORY != target->type) && (IO_VFS_MOUNT_POINT != target->type))
        return IO_NOT_A_DIRECTORY;
    
    if(NULL == target->child)
    {
        target->child = node;
        node->parent = target;
        return OK;
    }

    node->parent = target;
    target = target->child;
    while(target->next)
    {
        target = target->next;
    }
    target->next = node;
    node->previous = target;
    node->next = NULL;
    return OK;
}

bool IoVfsCheckIfNodeExists(struct IoVfsNode *parent, char *name)
{
    return (NULL != IoVfsGetNode(parent, name));
}

bool IoVfsCheckIfNodeExistsByPath(char *path)
{
    return (NULL != IoVfsGetNodeByPath(path));
}

STATUS IoVfsRemoveNode(struct IoVfsNode *node)
{
    ASSERT(node);
    if(node->flags & IO_VFS_FLAG_PERSISTENT)
        return IO_FILE_IRREMOVABLE;
    if(node->child)
        return IO_DIRECTORY_NOT_EMPTY;
    if(node->referenceCount)
        return IO_FILE_IN_USE;
    
    if(node->previous)
        node->previous->next = node->next;
    if(node->next)
        node->next->previous = node->previous;
    
    if(node->parent && (node->parent->child == node))
        node->parent->child = node->next;
    
    return OK;
}

STATUS IoVfsRead(struct IoVfsNode *node, IoVfsFlags flags, void *buffer, uint64_t size, uint64_t offset, IoReadWriteCompletionCallback callback, void *context)
{
    ASSERT(node && buffer && callback);

    if((IO_VFS_FILE != node->type) && (IO_VFS_DEVICE != node->type))
        return IO_BAD_FILE_TYPE;

    if(0 == size)
    {
        callback(OK, 0, context);
        return OK;
    }

    switch(node->fsType)
    {
        /* File operations on initial ramdisk are handled differently.
        The initrd driver is a static (standard compiled-in) driver 
        that does not provide standard driver interface 
        Initrd flag implies no caching and no writing.
        */
        case IO_VFS_FS_INITRD:
            callback(OK, InitrdReadFile(node->ref.u64, buffer, size, offset), context);
            return OK;
            break;
        case IO_VFS_FS_PHYSICAL:
        case IO_VFS_FS_VIRTUAL:
            return IoReadWrite(false, node->device, node, offset, size, buffer, callback, context);
            break;
        default:
            return IO_BAD_FILE_TYPE;
            break;
    }

    return IO_BAD_FILE_TYPE;
}

STATUS IoVfsWrite(struct IoVfsNode *node, IoVfsFlags flags, void *buffer, uint64_t size, uint64_t offset, uint64_t *actualSize)
{
    ASSERT(node && buffer && actualSize);
    if(0 == size)
    {
        *actualSize = 0;
        return OK;
    }

    *actualSize = 0;
    return IO_BAD_FILE_TYPE;
}

STATUS IoVfsCreateLink(char *path, char *linkDestination, IoVfsNodeFlags flags)
{
    ASSERT(path && linkDestination);
    if(IoVfsCheckIfNodeExistsByPath(path))
        return IO_FILE_ALREADY_EXISTS;
    
    char *file;
    file = IoVfsDetachFileName(path);

    struct IoVfsNode *parent = IoVfsGetNodeByPath(path);
    if(NULL == parent)
    {
        ERROR("Link path %s not found\n", path);
        return IO_FILE_NOT_FOUND;
    }
    if(IO_VFS_DIRECTORY != parent->type)
        return IO_NOT_A_DIRECTORY;

    struct IoVfsNode *destination = IoVfsGetNodeByPath(linkDestination);
    if(NULL == destination)
    {   
        ERROR("Link destination %s not found\n", linkDestination);
        return IO_FILE_NOT_FOUND;
    }
    
    struct IoVfsNode *link = IoVfsCreateNode(file);
    if(NULL == link)
        return OUT_OF_RESOURCES;
    
    link->flags = flags;
    link->type = IO_VFS_LINK;
    link->linkDestination = destination;
    link->linkDestination->referenceCount++;

    IoVfsInsertNode(link, parent);

    return OK;
}

STATUS IoVfsRemoveLink(char *path)
{
    ASSERT(path);
    struct IoVfsNode *link = IoVfsGetNodeByPath(path);
    if(NULL == link)
        return IO_FILE_NOT_FOUND;
    
    if(IO_VFS_LINK != link->type)
        return IO_BAD_FILE_TYPE;
    
    STATUS ret = IoVfsRemoveNode(link);
    if(OK != ret)
        return ret;

    if(link->linkDestination && link->linkDestination->referenceCount)
        link->linkDestination->referenceCount--;
        
    IoVfsDestroyNode(link);
    return OK;
}

STATUS IoVfsGetSize(char *path, uint64_t *size)
{
    ASSERT(path && size);
    struct IoVfsNode *n = IoVfsGetNodeByPath(path);
    if(NULL == n)
    {
        *size = 0;
        return IO_FILE_NOT_FOUND;
    }
    *size = n->size;
    return OK;
}

