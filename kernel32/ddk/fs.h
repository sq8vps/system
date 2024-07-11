#ifndef DDK_FS_H_
#define DDK_FS_H_

#include <stdint.h>
#include <stdbool.h>
#include "defines.h"

EXPORT
struct IoVfsNode;
struct IoDeviceObject;

EXPORT
/**
 * @brief Type specific operations for file systems
*/
enum FsOperations
{
    FS_NONE = 0,
    FS_GET_NODE, /**< Get VFS node of given file/directory */
    FS_GET_NODE_CHILDREN, /**< Get list of children VFS nodes for given node (directory, exclusively)*/
};

EXPORT
/**
 * @brief FS get node/get node children request body
 * 
 * This structure is used with \a FS_GET_NODE or \a FS_GET_NODE_CHILDREN.
 * When the operation code is \a FS_GET_NODE, then the kernel provides a pointer \a *parent to the parent VFS node
 * and a name of the node to be found. The driver then allocates a VFS node for the file/directory and returns
 * it through \a *node (it does NOT attach this node to the tree!). If such node does not exist, then NULL is returned
 * and the Request Packet should have the \a IO_FILE_NOT_FOUND status.
 * When the operation code is \a FS_GET_NODE_CHILDREN, then the kernel provides a pointer \a *node to the directory VFS node
 * and expects the driver to list all children nodes (files) in given directory, that is, create a linked list of
 * VFS nodes for all files and provide a pointer to the first child using \a *children. If there are no children,
 * it is set to NULL.
*/
union FsGetRequest
{
    struct
    {
        struct IoVfsNode *parent;
        char *name;
        struct IoVfsNode *node;
    } getNode;
    struct
    {
        struct IoVfsNode *node;
        struct IoVfsNode *children;
    } getChildren;
};

/**
 * @brief Get VFS node with given name and parent from the filesystem
 * @param *target Target filesystem device
 * @param *parent Parent VFS node
 * @param *name Node name
 * @param **node Output node or NULL if not found (check return code)
 * @return Status code
*/
INTERNAL STATUS FsGetNode(struct IoDeviceObject *target, struct IoVfsNode *parent, char *name, struct IoVfsNode **node);

/**
 * @brief Get VFS node children list
 * @param *target Target filesystem device
 * @param *node VFS node to find the children of
 * @param **children Output children list or NULL if no children (check return code)
 * @return Status code
*/
INTERNAL STATUS FsGetNodeChildren(struct IoDeviceObject *target, struct IoVfsNode *node, struct IoVfsNode **children);

#endif