#include "vfs.h"
#include "common.h"
#include "io/dev/dev.h"
#include "sdrv/initrd/initrd.h"
#include "mm/heap.h"
#include "assert.h"
#include "devfs.h"

#define IO_VFS_MAX_SYMLINK_DEPTH 40

static struct IoVfsNode *root = NULL; //filesystem root

struct IoVfsNode* IoVfsCreateNode(char *name)
{
    struct IoVfsNode *node = MmAllocateKernelHeapZeroed(sizeof(*node) + CmStrlen(name) + 1);
    if(NULL == node)
        return NULL;
    
    CmStrcpy(node->name, name);

    return node;
}

void IoVfsDestroyNode(struct IoVfsNode *node)
{
    ASSERT(node);
    
    MmFreeKernelHeap(node);
}

static STATUS purgeCache(struct IoVfsNode *node)
{
    return OK;
}

STATUS IoVfsInit(void)
{
    root = IoVfsCreateNode("");
    if(NULL == root)
        return IO_VFS_INITIALIZATION_FAILED;

    //prepare root node
    root->type = IO_VFS_DIRECTORY;
    root->child = NULL;
    root->flags = IO_VFS_FLAG_PERSISTENT;
    root->fsType = IO_VFS_FS_VIRTUAL;

    return IoInitDeviceFs(root);
}

char* IoVfsDetachFileName(char *path)
{
    ASSERT(path);
    char *c = path + CmStrlen(path);
    while(c >= path)
    {
        if('/' == *c)
        {
            *c = 0;
            return c + 1;
        }
        c--;
    }
    //no slash was found, whole path is the file name
    return path;
}

// /**
//  * @brief Resolve element: resolve link or load uncached element if required
//  * @param *target Target node. If NULL, then \a *targetName is used
//  * @param *targetName Target name string. Used when no \a *target is provided.
//  * @param *parent Parent node of the target
//  * @return Resolved element
//  * @note This function should be used to resolve all entries
// */
// static inline struct IoVfsNode* resolveElement(struct IoVfsNode *target, char *targetName, struct IoVfsNode *parent)
// {
//     ASSERT(target || targetName);
    
//     if(target) //there is a target node provided
//     {
//         //resolve symbolic links
//         if(IO_VFS_LINK == target->type)
//         {
//             uint16_t i = 0;
//             while(i < IO_VFS_MAX_SYMLINK_DEPTH)
//             {
//                 if(IO_VFS_LINK != target->type)
//                 {
//                     return resolveElement(target, target->name, target->parent);
//                 }
//                 target = target->linkDestination;
//                 if(NULL == target)
//                     return NULL;
//                 i++;
//             }
//             //max symbolic link resolution depth reached
//             return NULL;
//         }
//         //else just return unchanged target
//         return target;
//     }
//     else //target node not provided
//     {
//         if(NULL == parent)
//             return NULL;
        
//         uint64_t size = 0;
//         uint64_t ref = 0; 
//         switch(parent->majorType)
//         {
//             case IO_VFS_FS_INITRD:
//                 ref = InitrdGetFileReference(targetName, &size);
//                 if(0 != ref)
//                 {
//                     struct IoVfsNode *newNode = IoVfsCreateNode(targetName);
//                     if(NULL == newNode)
//                         return NULL;

//                     newNode->ref.u64 = ref;
//                     newNode->flags = IO_VFS_FLAG_NO_CACHE | IO_VFS_FLAG_READ_ONLY;
//                     newNode->majorType = IO_VFS_FS_INITRD;
//                     newNode->type = IO_VFS_FILE;
//                     newNode->size = size;
//                     return IoVfsInsertNode(newNode, parent);
//                 }
//                 break;
//             case IO_VFS_FS_TASKFS:
                
//                 break;
//         }

//         return NULL;
//     }
// }

// struct IoVfsNode* IoVfsGetNode(char *path)
// {
//     ASSERT(path);
//     uint32_t limit = IO_VFS_MAX_PATH_LENGTH;
//     struct IoVfsNode *node = &root;
//     char *element = NULL;

//     //resolve "" and "/", which point to the root
//     if('\0' == *path)
//         return &root;
    
//     if('/' == *path)
//     {
//         path++;

//         if('\0' == *path)
//             return node;
//     }

//     //resolve longer paths
//     element = path;
//     while(1)
//     {
//         if(('/' == *path) || ('\0' == *path))
//         {
//             //string passed to resolveElement() must be NULL-terminated
//             //save original character and replace it with NULL
//             char t = *path;
//             *path = 0;
//             //get child
//             node = resolveElement(node->child, element, node);
//             *path = t;
//             //find matching child
//             while(NULL != node)
//             {
//                 if(0 == CmStrncmp(element, node->name, (int)(path - element)))
//                 {
//                     //matching child found

//                     //this is the end of the path (either with or without '/')
//                     if((0 == *path) || (('/' == *path) && (0 == path[1])))
//                         return node;

//                     path++;
//                     element = path;
//                     break;
//                 }
//                 *path = 0;
//                 node = resolveElement(node->next, element, node->parent);
//                 *path = t;
//             }

//             if(NULL == node) //child not found
//             {
//                 return NULL;
//             }
//         }
//         path++;

//         if(0 == limit) //path length limit reached
//             return NULL;
//         limit--;   
//     }
//     return NULL;
// }

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
    struct IoVfsNode *node = root;

    //treat empty string as root
    if('\0' == path[0])
        return root;

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
        switch(node->fsType)
        {
            /* File operations on initial ramdisk are handled differently.
            The initrd driver is a static (standard compiled-in) driver 
            that does not provide standard driver interface 
            Initrd flag implies no caching and no writing.
            */
            case IO_VFS_FS_INITRD:
                *actualSize = InitrdReadFile(node->ref.u64, buffer, size, offset);
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

