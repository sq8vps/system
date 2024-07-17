#include "devfs.h"
#include "assert.h"
#include "ke/core/mutex.h"
#include "io/dev/dev.h"
#include "mm/mm.h"
#include "common.h"

static struct
{
    struct IoVfsNode *root;
}
IoDevfsState = {.root = NULL};

STATUS IoCreateDeviceFile(struct IoDeviceObject *dev, IoVfsNodeFlags flags, char *name)
{
    ASSERT(dev);
    ASSERT(IoDevfsState.root);
    ASSERT(name);
    STATUS status = OK;

    IoVfsLockTreeForWriting();
    if(!IoVfsCheckIfNodeExistsByParent(IoDevfsState.root, name))
    {
        struct IoVfsNode *node = IoVfsCreateNode(name);
        if(NULL != node)
        {
            node->type = IO_VFS_DEVICE;
            node->fsType = IO_VFS_FS_PHYSICAL;
            node->flags |= flags | IO_VFS_FLAG_NO_CACHE;
            node->device = dev;

            IoVfsInsertNode(node, IoDevfsState.root);
        }
        else
            status = OUT_OF_RESOURCES;
    }
    else
        status = IO_FILE_ALREADY_EXISTS;
        
    IoVfsUnlockTree();
    return status;
}

STATUS IoInitDeviceFs(struct IoVfsNode *root)
{
    IoDevfsState.root = IoVfsCreateNode("dev");
    if(NULL == IoDevfsState.root)
        return OUT_OF_RESOURCES;
    
    IoDevfsState.root->flags = IO_VFS_FLAG_PERSISTENT;
    IoDevfsState.root->type = IO_VFS_DIRECTORY;
    IoDevfsState.root->fsType = IO_VFS_FS_VIRTUAL;
    
    IoVfsInsertNode(IoDevfsState.root, root);
    
    return OK;
}