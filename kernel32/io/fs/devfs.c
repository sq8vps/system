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

    if(IoVfsCheckIfNodeExists(IoDevfsState.root, name))
        return IO_FILE_ALREADY_EXISTS;

    struct IoVfsNode *node = IoVfsCreateNode(name);
    if(NULL == node)
    {
        return OUT_OF_RESOURCES;
    }
    node->type = IO_VFS_DEVICE;
    node->fsType = IO_VFS_FS_PHYSICAL;
    node->flags |= flags | IO_VFS_FLAG_NO_CACHE;
    node->device = dev;

    IoVfsInsertNode(node, IoDevfsState.root);
    return OK;
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