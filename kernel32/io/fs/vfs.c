#include "vfs.h"
#include "rtl/string.h"
#include "io/dev/dev.h"
#include "io/initrd.h"
#include "assert.h"
#include "devfs.h"
#include "mm/slab.h"
#include "io/dev/rp.h"
#include "ddk/fs.h"
#include "mm/heap.h"
#include "rtl/stdlib.h"

#define IO_VFS_MAX_SYMLINK_DEPTH 10
#define IO_VFS_DEFAULT_MAX_FILE_NAME_LENGTH 127
#define IO_VFS_CHUNKS_PER_SLAB 16

static struct
{
    struct IoVfsNode *root; /**< Root node */
    void *slabHandle; /**< VFS node slab allocator handle */
    uint32_t maxFileNameLength; /**< Max file name length */
    KeRwLock lock; /**< Global VFS tree RW lock */
} 
IoVfsState = {.root = NULL, .slabHandle = NULL, .maxFileNameLength = IO_VFS_DEFAULT_MAX_FILE_NAME_LENGTH, .lock = KeRwLockInitializer};


struct IoVfsNode* IoVfsCreateNode(const char *name)
{
    if(RtlStrlen(name) > IoVfsState.maxFileNameLength)
        return NULL;
    
    if(!RtlCheckFileName(name))
        return NULL;

    struct IoVfsNode *node = MmSlabAllocate(IoVfsState.slabHandle);
    if(NULL == node)
        return NULL;
    
    RtlMemset(node, 0, sizeof(*node));
    
    ObInitializeObjectHeader(node);
    
    RtlStrncpy(node->name, name, IoVfsState.maxFileNameLength);

    node->lock = (KeRwLock)KeRwLockInitializer;

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
        return VFS_INITIALIZATION_FAILED;

    //prepare root node
    IoVfsState.root->type = IO_VFS_DIRECTORY;
    IoVfsState.root->child = NULL;
    IoVfsState.root->flags = IO_VFS_FLAG_PERSISTENT;
    IoVfsState.root->fsType = IO_VFS_FS_VIRTUAL;
    IoVfsState.root->status.open = 1;

    return IoInitDeviceFs(IoVfsState.root);
}

STATUS IoVfsOpen(struct IoVfsNode *node, bool write, IoFileFlags flags)
{
    ASSERT(node);
    STATUS status = OK;

    if((IO_VFS_DIRECTORY == node->type) || (IO_VFS_MOUNT_POINT == node->type))
        return BAD_FILE_TYPE;

    if(flags & IO_FILE_FLAG_NO_WAIT)
    {
        if(!KeAcquireRwLockWithTimeout(&(node->lock), write, KE_MUTEX_NO_WAIT))
            return FILE_LOCKED;
    }
    else
        KeAcquireRwLock(&(node->lock), write);

    if(write)
        node->references.writers++;
    else
        node->references.readers++;

    if(!node->status.open)
    {
        //ask filesystem to open the file
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
                status = BAD_FILE_TYPE;
                break;
        }
    }

    if(OK != status)
    {
        //clean up on open failure
        if(write)
            node->references.writers--;
        else
            node->references.readers--;
        
        KeReleaseRwLock(&(node->lock));
    }
    else
    {
        node->status.open = 1;
    }

    return status;
}

STATUS IoVfsClose(struct IoVfsNode *node)
{
    ASSERT(node);
    STATUS status = OK;

    if((IO_VFS_DIRECTORY == node->type) || (IO_VFS_MOUNT_POINT == node->type))
        return BAD_FILE_TYPE;

    if(node->references.readers)
        node->references.readers--;
    else if(node->references.writers)
        node->references.writers--;

    if(node->flags & IO_VFS_FLAG_PERSISTENT)
    {
        KeReleaseRwLock(&(node->lock));
        return OK;
    }

    //check if there are other readers currently using this file or if there are any tasks waiting
    //if not, then close the file
    if((0 == node->references.readers) && (0 == node->references.writers))
    {
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
                status = BAD_FILE_TYPE;
                break;
        }

        if(OK == status)
        {
            node->status.open = 0;
        }
    }

    KeReleaseRwLock(&(node->lock));
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

static inline struct IoVfsNode *IoVfsGetNodeFromCache(const struct IoVfsNode *parent, const char *name)
{
    ASSERT(parent && name);

    //"." file, which points to the same directory
    if(0 == RtlStrcmp(name, "."))
        return (struct IoVfsNode*)parent;
    
    //".." file, which points to parent directory
    if(0 == RtlStrcmp(name, ".."))
        return parent->parent;

    struct IoVfsNode *node = parent->child;

    //look for given child
    while(node)
    {
        if(0 == RtlStrcmp(node->name, name))
            return node;
        
        node = node->next;
    }
    return NULL;
}

static struct IoVfsNode *IoVfsGetNodeByParent(struct IoVfsNode *parent, const char *name)
{
    ASSERT(parent && name);
    STATUS status = OK;

    struct IoVfsNode *node = IoVfsGetNodeFromCache(parent, name);
    if(NULL != node)
        return node;

    //child not found in cached tree, must ask driver to find it on a physical storage

    switch(parent->fsType)
    {
        case IO_VFS_FS_PHYSICAL:
            status = FsGetNode(parent, name, &node);
            break;
        case IO_VFS_FS_INITRD:
            status = IoInitrdGetNode(parent, name, &node);
            break;
        case IO_VFS_FS_TASKFS:
            return NULL;
            break;
        default:
            return NULL;
            break;
    }

    if(OK == status)
    {
        IoVfsInsertNode(node, parent);
        return node;
    }
    else
        return NULL;
}

struct IoVfsNode *IoVfsGetNodeEx(const char *path, bool excludeLastElement)
{
    struct IoVfsNode *node = IoVfsState.root;

    //treat empty string as root
    if('\0' == path[0])
        return IoVfsState.root;

    //every path must start from root
    if('/' != path[0])
        return NULL;
    
    //skip '/'
    ++path;

    char *buffer = MmAllocateKernelHeap(RtlStrlen(path) + 1);
    if(NULL == buffer)
        return NULL;
    
    char *p = buffer;
    
    RtlStrcpy(p, path);

    while('\0' != *p)
    {
        char *element = p;
        //find separator or end of path
        while(('/' != *p) && ('\0' != *p))
        {
            ++p;
        }
        if(excludeLastElement && (('\0' == p[0])
            || (('/' == p[0]) && ('\0' == p[1]))))
        {
            return node;
        }
        char rep = *p;
        *p = '\0'; //tokenize
        node = IoVfsGetNodeByParent(node, element);
        //check if node was found
        if(NULL == node)
        {
            break;
        }
        *p = rep;

        //check if this is the end of the path
        //treat ".../file/" the same as ".../file", doesn't matter if it's a directory or not
        if(('\0' == p[0])
            || (('/' == p[0]) && ('\0' == p[1])))
        {
            break;
        }

        //go to the next character
        ++p;

        //now check if given node is a symbolic link
        //if so, then resolve
        if(IO_VFS_LINK == node->type)
        {
            node = IoVfsResolveLink(node);
            if(NULL == node)
                break;
        }
    }

    MmFreeKernelHeap(buffer);
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

STATUS IoVfsInsertNodeByPath(struct IoVfsNode *node, const char *path, bool isFilePath)
{
    ASSERT(node && path);
    if(isFilePath)
    {
        if(0 != RtlStrcmp(node->name, RtlGetFileName(path))) //check for node name and path file name match
            return FILE_NOT_FOUND;
    }
    struct IoVfsNode *target = IoVfsGetNodeEx(path, isFilePath);
    if(NULL == target)
    {
        return FILE_NOT_FOUND;
    }
    
    if((IO_VFS_DIRECTORY != target->type) && (IO_VFS_MOUNT_POINT != target->type))
        return NOT_A_DIRECTORY;

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

bool IoVfsCheckIfNodeExists(const char *path)
{
    IoVfsLockTreeForWriting();
    bool status = (NULL != IoVfsGetNode(path));
    IoVfsUnlockTree();
    return status;
}

bool IoVfsCheckIfNodeExistsByParent(struct IoVfsNode *parent, const char *name)
{
    return (NULL != IoVfsGetNodeByParent(parent, name));
}

STATUS IoVfsRemoveNode(struct IoVfsNode *node)
{
    ASSERT(node);
    STATUS status = OK;

    if(node->flags & IO_VFS_FLAG_PERSISTENT)
        status = FILE_IRREMOVABLE;
    else if(node->child)
        status = DIRECTORY_NOT_EMPTY;
    else if(node->references.readers || node->references.writers)
        status = FILE_IN_USE;
    else
    {
        if(node->previous)
            node->previous->next = node->next;
        if(node->next)
            node->next->previous = node->previous;
        
        if(node->parent && (node->parent->child == node))
            node->parent->child = node->next;
    }
    
    return status;
}

STATUS IoVfsRead(struct IoVfsNode *node, IoFileFlags flags, void *buffer, size_t size, uint64_t offset, IoReadWriteCompletionCallback callback, void *context)
{
    ASSERT(node && buffer && callback);

    if(!node->status.open)
        return FILE_CLOSED;

    if((IO_VFS_FILE != node->type) && (IO_VFS_DEVICE != node->type) && (IO_VFS_LINK != node->type))
        return BAD_FILE_TYPE;

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
            callback(OK, IoInitrdRead(node, buffer, size, offset), context);
            return OK;
            break;
        case IO_VFS_FS_PHYSICAL:
        case IO_VFS_FS_VIRTUAL:
            return IoReadWrite(false, node->device, node, offset, size, buffer, callback, context, 
                (flags & IO_FILE_FLAG_DIRECT) ? true : false);
            break;
        default:
            return BAD_FILE_TYPE;
            break;
    }

    return BAD_FILE_TYPE;
}

STATUS IoVfsWrite(struct IoVfsNode *node, IoFileFlags flags, void *buffer, size_t size, uint64_t offset, IoReadWriteCompletionCallback callback, void *context)
{
    ASSERT(node && buffer && callback);
    STATUS status = 0;

    if(!node->status.open)
        return FILE_CLOSED;

    if((IO_VFS_FILE != node->type) && (IO_VFS_DEVICE != node->type) && (IO_VFS_LINK != node->type))
        return BAD_FILE_TYPE;

    if(0 == size)
    {
        callback(OK, 0, context);
        return OK;
    }

    //allow only overwriting or appending, no empty spaces
    //do not apply this to devices
    if((offset > node->size) && (IO_VFS_DEVICE != node->type))
        return FILE_TOO_SMALL;

    size_t originalSize = node->size;
    //TODO: update timestamps
    if((offset + size) > node->size)
    {
        node->size = offset + size;
    }

    switch(node->fsType)
    {
        /* File operations on initial ramdisk are handled differently.
        The initrd driver is a static (standard compiled-in) driver 
        that does not provide standard driver interface 
        Initrd flag implies no caching and no writing.
        */
        case IO_VFS_FS_INITRD:
            status = FILE_READ_ONLY;
            break;
        case IO_VFS_FS_PHYSICAL:
        case IO_VFS_FS_VIRTUAL:
            status = IoReadWrite(true, node->device, node, offset, size, buffer, callback, context, 
                (flags & IO_FILE_FLAG_DIRECT) ? true : false);
            break;
        default:
            status = BAD_FILE_TYPE;
            break;
    }
    if(OK != status)
        node->size = originalSize;

    return status;
}

STATUS IoVfsCreateLink(char *path, char *linkDestination, IoVfsNodeFlags flags)
{
    ASSERT(path && linkDestination);
    if(IoVfsCheckIfNodeExists(path))
        return FILE_ALREADY_EXISTS;

    IoVfsLockTreeForWriting();

    struct IoVfsNode *parent = IoVfsGetNodeEx(path, true);
    if(NULL == parent)
    {
        IoVfsUnlockTree();
        return FILE_NOT_FOUND;
    }

    if(IO_VFS_DIRECTORY != parent->type)
    {
        IoVfsUnlockTree();
        return NOT_A_DIRECTORY;
    }

    struct IoVfsNode *destination = IoVfsGetNode(linkDestination);
    if(NULL == destination)
    {   
        IoVfsUnlockTree();
        return FILE_NOT_FOUND;
    }
    
    struct IoVfsNode *link = IoVfsCreateNode(RtlGetFileName(path));
    if(NULL == link)
    {
        IoVfsUnlockTree();
        return OUT_OF_RESOURCES;
    }
    
    link->flags = flags;
    link->type = IO_VFS_LINK;
    link->linkDestination = destination;
    destination->references.links++;
    //link->linkDestination->referenceCount++;

    IoVfsInsertNode(link, parent);

    IoVfsUnlockTree();

    return OK;
}

STATUS IoVfsRemoveLink(char *path)
{
    // ASSERT(path);
    // struct IoVfsNode *link = IoVfsGetNode(path);
    // if(NULL == link)
    //     return FILE_NOT_FOUND;
    
    // if(IO_VFS_LINK != link->type)
    //     return BAD_FILE_TYPE;
    
    // STATUS ret = IoVfsRemoveNode(link);
    // if(OK != ret)
    //     return ret;

    // // if(link->linkDestination && link->linkDestination->referenceCount)
    // //     link->linkDestination->referenceCount--;
        
    // IoVfsDestroyNode(link);
    return OK;
}

STATUS IoVfsGetSize(const char *path, uint64_t *size)
{
    ASSERT(path && size);
    
    IoVfsLockTreeForWriting();
    struct IoVfsNode *n = IoVfsGetNode(path);
    if(NULL == n)
    {
        IoVfsUnlockTree();
        *size = 0;
        return FILE_NOT_FOUND;
    }
    *size = n->size;
    IoVfsUnlockTree();
    return OK;
}

void IoVfsLockTreeForReading(void)
{
    KeAcquireRwLock(&(IoVfsState.lock), false);
}

void IoVfsLockTreeForWriting(void)
{
    KeAcquireRwLock(&(IoVfsState.lock), true);
}

void IoVfsUnlockTree(void)
{
    KeReleaseRwLock(&(IoVfsState.lock));
}