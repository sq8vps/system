#include "taskfs.h"
#include "assert.h"
#include "vfs.h"
#include "rtl/stdlib.h"
#include "rtl/ctype.h"
#include "ke/sched/sched.h"

static struct
{
    struct IoVfsNode *root; /**< Task filesystem root (/task) */
    struct
    {
        struct IoVfsNode *root; /**< Process self directory (/task/self) */
        struct
        {
            struct IoVfsNode *root; /**< Process file descriptor directory (/task/self/fd) */
            struct IoVfsNode *file; /**< File link abstraction (/task/self/fd/xxx) */
        } fd;
    } self;
}
IoTaskFsState = {.root = NULL, .self.root = NULL, .self.fd.root = NULL, .self.fd.file = NULL};


STATUS IoInitTaskFs(struct IoVfsNode *root)
{
    IoTaskFsState.root = IoVfsCreateNode("task");
    if(NULL == IoTaskFsState.root)
        return OUT_OF_RESOURCES;
    
    IoTaskFsState.root->flags = IO_VFS_FLAG_PERSISTENT;
    IoTaskFsState.root->type = IO_VFS_DIRECTORY;
    IoTaskFsState.root->fsType = IO_VFS_FS_VIRTUAL;
    
    IoVfsInsertNode(IoTaskFsState.root, root);

    IoTaskFsState.self.root = IoVfsCreateNode("self");
    if(NULL == IoTaskFsState.self.root)
        return OUT_OF_RESOURCES;
    
    IoTaskFsState.self.root->flags = IO_VFS_FLAG_PERSISTENT;
    IoTaskFsState.self.root->type = IO_VFS_DIRECTORY;
    IoTaskFsState.self.root->fsType = IO_VFS_FS_TASKFS;
    
    IoVfsInsertNode(IoTaskFsState.self.root, IoTaskFsState.root);

    IoTaskFsState.self.fd.root = IoVfsCreateNode("fd");
    if(NULL == IoTaskFsState.self.fd.root)
        return OUT_OF_RESOURCES;
    
    IoTaskFsState.self.fd.root->flags = IO_VFS_FLAG_PERSISTENT;
    IoTaskFsState.self.fd.root->type = IO_VFS_DIRECTORY;
    IoTaskFsState.self.fd.root->fsType = IO_VFS_FS_TASKFS;
    
    IoVfsInsertNode(IoTaskFsState.self.fd.root, IoTaskFsState.self.root);

    IoTaskFsState.self.fd.file = IoVfsCreateNode("n");
    if(NULL == IoTaskFsState.self.fd.file)
        return OUT_OF_RESOURCES;
    
    IoTaskFsState.self.fd.file->flags = IO_VFS_FLAG_PERSISTENT;
    IoTaskFsState.self.fd.file->type = IO_VFS_LINK;
    IoTaskFsState.self.fd.file->fsType = IO_VFS_FS_TASKFS;
    
    IoVfsInsertNode(IoTaskFsState.self.fd.file, IoTaskFsState.self.fd.root);
    
    return OK;
}

STATUS IoTaskFsGetNode(const struct IoVfsNode *parent, const char *name, struct IoTaskFsContext *data, struct IoVfsNode **node)
{
    struct KeTaskControlBlock *tcb = KeGetCurrentTask();
    struct KeProcessControlBlock *pcb = tcb->parent;

    if(IoTaskFsState.root == parent)
    {
        //TODO: look for task with given TID
        return FILE_NOT_FOUND;
    }
    else if(IoTaskFsState.self.fd.root == parent)
    {
        if(NULL != data)
            data->tid = tcb->tid;

        //looking for file link under /task/self/fd
        const char *t = name;
        while('\0' != *t)
        {
            if(!RtlIsdigit(*t))
                return FILE_NOT_FOUND;
            ++t;
        }
        int fd = RtlAtoi(name);
        if(NULL != IoGetVfsNodeForFile(pcb, fd))
        {
            if(NULL != data)
                data->fd = fd;
            *node = IoTaskFsState.self.fd.file;
            return OK;
        }
        else
            return FILE_NOT_FOUND;
    }
    else
    {
        return FILE_NOT_FOUND;
    }
}

struct IoVfsNode* IoTaskFsResolveLink(struct IoVfsNode *link, struct IoTaskFsContext *data)
{
    if(IO_VFS_LINK != link->type)
        return link;

    if(NULL == data) //meaningless without taskfs context
        return NULL;
    
    struct KeTaskControlBlock *tcb = KeGetCurrentTask();
    struct KeProcessControlBlock *pcb = tcb->parent;

    if(IoTaskFsState.self.fd.file == link)
    {
        data->tid = tcb->tid;
        return IoGetVfsNodeForFile(pcb, data->fd);
    }
    else
    {
        return NULL;
    }
}