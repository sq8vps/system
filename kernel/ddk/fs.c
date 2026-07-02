#include "fs.h"
#include "io/dev/rp.h"
#include "io/dev/dev.h"
#include "mm/heap.h"
#include "io/fs/vfs.h"

STATUS FsGetNode(const struct IoVfsNode *parent, const char *name, struct IoVfsNode **node)
{
    STATUS status = OK;

    if(!node)
        return BAD_PARAMETER;
    
    if(IO_DEVICE_TYPE_FS != parent->device->type)
        return BAD_TYPE;
    
    struct IoRp *rp = IoCreateRp();
    if(NULL == rp)
        return OUT_OF_RESOURCES;
    
    union FsRequest *req = MmAllocateKernelHeap(sizeof(union FsRequest));
    if(NULL == req)
    {
        IoFreeRp(rp);
        return OUT_OF_RESOURCES;
    }

    rp->code = IO_RP_FILESYSTEM_CONTROL;
    rp->payload.deviceControl.code = FS_GET_NODE;
    rp->payload.deviceControl.data = req;

    req->getNode.parent = parent;
    req->getNode.name = name;
    
    status = IoSendRpSync(parent->device, rp);
    if(OK == status)
        *node = req->getNode.node;

    IoFreeRp(rp);
    MmFreeKernelHeap(req);
    return status;
}

STATUS FsGetNodeChildren(const struct IoVfsNode *node, struct IoVfsNode **children)
{
    STATUS status = OK;

    if(!node)
        return BAD_PARAMETER;
    
    if(IO_DEVICE_TYPE_FS != node->device->type)
        return BAD_TYPE;
    
    struct IoRp *rp = IoCreateRp();
    if(NULL == rp)
        return OUT_OF_RESOURCES;
    
    union FsRequest *req = MmAllocateKernelHeap(sizeof(union FsRequest));
    if(NULL == req)
    {
        IoFreeRp(rp);
        return OUT_OF_RESOURCES;
    }

    rp->code = IO_RP_FILESYSTEM_CONTROL;
    rp->payload.deviceControl.code = FS_GET_NODE_CHILDREN;
    rp->payload.deviceControl.data = req;

    req->getChildren.node = node;
    
    status = IoSendRpSync(node->device, rp);
    if(OK == status)
        *children = req->getChildren.children;

    IoFreeRp(rp);
    MmFreeKernelHeap(req);
    return status;
}

STATUS FsCreateFile(const struct IoVfsNode *parent, const char *name, enum IoVfsEntryType type, enum IoVfsFlags flags, struct IoVfsNode **node)
{
    STATUS status = OK;

    if(!node || !name)
        return BAD_PARAMETER;
    
    if(IO_DEVICE_TYPE_FS != parent->device->type)
        return BAD_TYPE;
    
    struct IoRp *rp = IoCreateRp();
    if(NULL == rp)
        return OUT_OF_RESOURCES;
    
    union FsRequest *req = MmAllocateKernelHeap(sizeof(*req));
    if(NULL == req)
    {
        IoFreeRp(rp);
        return OUT_OF_RESOURCES;
    }

    rp->code = IO_RP_FILESYSTEM_CONTROL;
    rp->payload.deviceControl.code = FS_CREATE;
    rp->payload.deviceControl.data = req;

    req->create.parent = parent;
    req->create.name = name;
    req->create.type = type;
    req->create.flags = flags;
    
    status = IoSendRpSync(parent->device, rp);
    if(OK == status)
        *node = req->create.node;

    IoFreeRp(rp);
    MmFreeKernelHeap(req);
    return status;
}