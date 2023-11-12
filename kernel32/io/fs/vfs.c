#include "vfs.h"
#include "common.h"
#include "io/devmgr/dev.h"
#include "sdrv/initrd/initrd.h"
#include "mm/heap.h"
#include "assert.h"

#define IO_VFS_MAX_SYMLINK_DEPTH 40

static struct IoVfsNode root; //filesystem root

static inline void clearNode(struct IoVfsNode *node)
{
    CmMemset(node, 0, sizeof(*node));
}

struct IoVfsNode* IoVfsCreateNode(char *name)
{
    struct IoVfsNode *node = MmAllocateKernelHeap(sizeof(struct IoVfsNode));
    if(NULL == node)
        return NULL;
    
    clearNode(node);

    if(NULL == (node->name = MmAllocateKernelHeap(CmStrlen(name) + 1)))
    {
        MmFreeKernelHeap(node);
        return NULL;
    }
    CmStrcpy(node->name, name);

    return node;
}

void IoVfsDestroyNode(struct IoVfsNode *node)
{
    ASSERT(node);
    if(node->name)
        MmFreeKernelHeap(node->name);
    
    MmFreeKernelHeap(node);
}

static STATUS purgeCache(struct IoVfsNode *node)
{
    return OK;
}

STATUS IoVfsInit(void)
{
    clearNode(&root);

    //prepare root node
    root.name = "";
    root.type = IO_VFS_DIRECTORY;
    root.child = NULL;
    root.flags = IO_VFS_FLAG_PERSISTENT;
    root.majorType = IO_VFS_FS_VIRTUAL;

    //prepare dev subdirectory
    struct IoVfsNode *dev = IoVfsCreateNode("dev");
    if(NULL == dev)
        return OUT_OF_RESOURCES;
    
    dev->flags = IO_VFS_FLAG_PERSISTENT;
    dev->type = IO_VFS_DIRECTORY;
    dev->majorType = IO_VFS_FS_VIRTUAL;
    
    if(dev != IoVfsInsertNode(dev, &root))
        return IO_VFS_INITIALIZATION_FAILED;

    return OK;
}

/**
 * @brief Detach file name from given path
 * @param *path Input path and output path without file name
 * @param **file Detached file name
*/
static void detachFileName(char *path, char **file)
{
    ASSERT(path && *file);
    char *c = path + CmStrlen(path);
    while(c >= path)
    {
        if('/' == *c)
        {
            *file = c + 1;
            *c = 0;
            return;
        }
        c--;
    }
    *file = path;
}

/**
 * @brief Resolve element: resolve link or load uncached element if required
 * @param *target Target node. If NULL, then \a *targetName is used
 * @param *targetName Target name string. Used when no \a *target is provided.
 * @param *parent Parent node of the target
 * @return Resolved element
 * @note This function should be used to resolve all entries
*/
static inline struct IoVfsNode* resolveElement(struct IoVfsNode *target, char *targetName, struct IoVfsNode *parent)
{
    ASSERT(target || targetName);
    
    if(target) //there is a target node provided
    {
        //resolve symbolic links
        if(IO_VFS_LINK == target->type)
        {
            uint16_t i = 0;
            while(i < IO_VFS_MAX_SYMLINK_DEPTH)
            {
                if(IO_VFS_LINK != target->type)
                {
                    return resolveElement(target, target->name, target->parent);
                }
                target = target->linkDestination;
                if(NULL == target)
                    return NULL;
                i++;
            }
            //max symbolic link resolution depth reached
            return NULL;
        }
        //else just return unchanged target
        return target;
    }
    else //target node not provided
    {
        if(NULL == parent)
            return NULL;
        
        uint64_t size = 0;
        uint64_t ref = 0; 
        switch(parent->majorType)
        {
            case IO_VFS_FS_INITRD:
                ref = InitrdGetFileReference(targetName, &size);
                if(0 != ref)
                {
                    struct IoVfsNode *newNode = IoVfsCreateNode(targetName);
                    if(NULL == newNode)
                        return NULL;

                    newNode->referenceValue = ref;
                    newNode->flags = IO_VFS_FLAG_NO_CACHE | IO_VFS_FLAG_READ_ONLY;
                    newNode->majorType = IO_VFS_FS_INITRD;
                    newNode->type = IO_VFS_FILE;
                    newNode->size = size;
                    return IoVfsInsertNode(newNode, parent);
                }
                break;
            case IO_VFS_FS_TASKFS:
                
                break;
        }

        return NULL;
    }
}



struct IoVfsNode* IoVfsGetNode(char *path)
{
    ASSERT(path);
    uint32_t limit = IO_VFS_MAX_PATH_LENGTH;
    struct IoVfsNode *node = &root;
    char *element = NULL;
    if(0 == *path)
        return &root;
    
    if('/' == *path)
    {
        path++;

        if(0 == *path)
            return node;
    }

    
    element = path;
    while(1)
    {
        if(('/' == *path) || (0 == *path))
        {
            //string passed to resolveElement() must be NULL-terminated
            //save original character and replace it with NULL
            char t = *path;
            *path = 0;
            //get child
            node = resolveElement(node->child, element, node);
            *path = t;
            //find matching child
            while(NULL != node)
            {
                if(0 == CmStrncmp(element, node->name, (int)(path - element)))
                {
                    //matching child found

                    //this is the end of the path (either with or without '/')
                    if((0 == *path) || (('/' == *path) && (0 == path[1])))
                        return node;

                    path++;
                    element = path;
                    break;
                }
                *path = 0;
                node = resolveElement(node->next, element, node->parent);
                *path = t;
            }

            if(NULL == node) //child not found
            {
                return NULL;
            }
        }
        path++;

        if(0 == limit) //path length limit reached
            return NULL;
        limit--;   
    }
    return NULL;
}

struct IoVfsNode* IoVfsInsertNode(struct IoVfsNode *node, struct IoVfsNode *parent)
{
    ASSERT(node && parent);
    
    node->parent = parent;
    node->previous = NULL;

    //find next free position
    if(parent->child)
    {
        struct IoVfsNode *previous = parent->child;
        while(previous->next)
            previous = previous->next;
        
        previous->next = node;
        node->previous = previous;
    }
    else
        parent->child = node;

    node->next = NULL;
    return node;
}

STATUS IoVfsInsertNodeByPath(struct IoVfsNode *node, char *path)
{
    ASSERT(node && path);
    struct IoVfsNode *target = IoVfsGetNode(path);
    if(NULL == target)
    {
        ERROR("Node %s not found\n", path);
        return IO_FILE_NOT_FOUND;
    }
    
    if((IO_VFS_DIRECTORY != target->type) && (IO_VFS_DISK != target->type))
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

STATUS IoVfsRead(struct IoVfsNode *node, IoVfsOperationFlags flags, void *buffer, uint64_t size, uint64_t offset, uint64_t *actualSize)
{
    ASSERT(node && buffer && actualSize);
    if(0 == size)
    {
        *actualSize = 0;
        return OK;
    }
    if(IO_VFS_FILE == node->type)
    {

        switch(node->majorType)
        {
            /* File operations on initial ramdisk are handled differently.
            The initrd driver is a static (standard compiled-in) driver 
            that does not provide standard driver interface 
            Initrd flag implies no caching and no writing.
            */
            case IO_VFS_FS_INITRD:
                *actualSize = InitrdReadFile(node->referenceValue, buffer, size, offset);
                return OK;
        }
    }
    else if(IO_VFS_DEVICE == node->type)
    {
        *actualSize = 0;
        return NOT_IMPLEMENTED;
    }

    *actualSize = 0;
    return IO_BAD_FILE_TYPE;
}

STATUS IoVfsWrite(struct IoVfsNode *node, IoVfsOperationFlags flags, void *buffer, uint64_t size, uint64_t offset, uint64_t *actualSize)
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

STATUS IoVfsCreateLink(char *path, char *linkDestination, IoVfsFlags flags)
{
    ASSERT(path && linkDestination);
    if(NULL != IoVfsGetNode(path))
        return IO_FILE_ALREADY_EXISTS;
    
    char *file;
    detachFileName(path, &file);

    struct IoVfsNode *parent = IoVfsGetNode(path);
    if(NULL == parent)
    {
        ERROR("Link path %s not found\n", path);
        return IO_FILE_NOT_FOUND;
    }
    if(IO_VFS_DIRECTORY != parent->type)
        return IO_NOT_A_DIRECTORY;

    struct IoVfsNode *destination = IoVfsGetNode(linkDestination);
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
    struct IoVfsNode *link = IoVfsGetNode(path);
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
    struct IoVfsNode *n =IoVfsGetNode(path);
    if(NULL == n)
    {
        *size = 0;
        return IO_FILE_NOT_FOUND;
    }
    *size = n->size;
    return OK;
}
