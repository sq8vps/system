#include "fs.h"
#include "io/dev/rp.h"
#include "io/dev/dev.h"
#include "mm/heap.h"
#include "assert.h"
#include "io/fs/vfs.h"

STATUS FsGetNode(const struct IoVfsNode *parent, const char *name, struct IoVfsNode **node)
{
    STATUS status = OK;

    if(!node)
        return NULL_POINTER_GIVEN;
    
    if(IO_DEVICE_TYPE_FS != parent->device->type)
        return IO_DEVICE_INCOMPATIBLE;
    
    struct IoRp *rp = IoCreateRp();
    if(NULL == rp)
        return OUT_OF_RESOURCES;
    
    union FsGetRequest *req = MmAllocateKernelHeap(sizeof(union FsGetRequest));
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
    
    status = IoSendRp(parent->device, rp);
    if(OK == status)
    {
        IoWaitForRpCompletion(rp);
        status = rp->status;
        if(OK == status)
            *node = req->getNode.node;
    }

    IoFreeRp(rp);
    MmFreeKernelHeap(req);
    return status;
}

STATUS FsGetNodeChildren(const struct IoVfsNode *node, struct IoVfsNode **children)
{
    STATUS status = OK;

    if(!node)
        return NULL_POINTER_GIVEN;
    
    if(IO_DEVICE_TYPE_FS != node->device->type)
        return IO_DEVICE_INCOMPATIBLE;
    
    struct IoRp *rp = IoCreateRp();
    if(NULL == rp)
        return OUT_OF_RESOURCES;
    
    union FsGetRequest *req = MmAllocateKernelHeap(sizeof(union FsGetRequest));
    if(NULL == req)
    {
        IoFreeRp(rp);
        return OUT_OF_RESOURCES;
    }

    rp->code = IO_RP_FILESYSTEM_CONTROL;
    rp->payload.deviceControl.code = FS_GET_NODE_CHILDREN;
    rp->payload.deviceControl.data = req;

    req->getChildren.node = node;
    
    status = IoSendRp(node->device, rp);
    if(OK == status)
    {
        IoWaitForRpCompletion(rp);
        status = rp->status;
        if(OK == status)
            *children = req->getChildren.children;
    }

    IoFreeRp(rp);
    MmFreeKernelHeap(req);
    return status;
}
