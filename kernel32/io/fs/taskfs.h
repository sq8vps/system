#ifndef KERNEL_TASKFS_H_
#define KERNEL_TASKFS_H_

#include "defines.h"
#include "ke/task/task.h"

struct IoVfsNode;

EXPORT_API

/**
 * @brief Task file system context used for accessing /taskfs
 */
struct IoTaskFsContext
{
    KE_TASK_ID tid; 
    int fd;
};

#define IO_TASK_FS_CONTEXT_INITIALIZER {.tid = -1, .fd = -1}

END_EXPORT_API

/**
 * @brief Get VFS node from task file system
 * @param *parent Parent VFS node
 * @param *name Name of the node to look for
 * @param *data Task FS context
 * @param **node Output VFS node. As task fs is an abstract filesystem, 
 * the returned node actually belongs to a different file system or is virtual and must not be inserted.
 * @return Status code
 */
INTERNAL STATUS IoTaskFsGetNode(const struct IoVfsNode *parent, const char *name, struct IoTaskFsContext *data, struct IoVfsNode **node);

/**
 * @brief Resolve link from task filesystem
 * @param *link Link node
 * @param *data Task FS context
 * @return Destination node
 */
INTERNAL struct IoVfsNode* IoTaskFsResolveLink(struct IoVfsNode *link, struct IoTaskFsContext *data);

/**
 * @brief Initialize task file system
 * @param *root Root file system node
 * @return Status code
 */
INTERNAL STATUS IoInitTaskFs(struct IoVfsNode *root);

#endif