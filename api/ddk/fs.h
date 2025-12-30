//This header file is generated automatically
#ifndef EXPORTED___API__DDK_FS_H_
#define EXPORTED___API__DDK_FS_H_

#ifdef __cplusplus
extern "C" 
{
#endif

#include <stdint.h>
#include <stdbool.h>
#include "defines.h"

struct IoVfsNode;
struct IoDeviceObject;

/**
 * @addtogroup ddk_fs File system requests and helpers
 * @ingroup ddk
 * 
 * @{
 */

/**
 * @brief Type specific operations for file systems
*/
enum FsOperations
{
    FS_NONE = 0, /**< No operation */
    FS_GET_NODE = 1, /**< Get VFS node of given file/directory */
    FS_GET_NODE_CHILDREN = 2, /**< Get list of children VFS nodes for given node (directory, exclusively)*/
};


/**
 * @brief Get node/get node children from file system request body
 * 
 * This structure is used with #FS_GET_NODE or #FS_GET_NODE_CHILDREN.
 * When the operation code is #FS_GET_NODE, then the kernel provides a pointer \a *parent to the parent VFS node
 * and a name of the node to be found. The driver then allocates a VFS node for the file/directory and returns
 * it through \a *node (it does NOT attach this node to the tree!). If such node does not exist, then NULL is returned
 * and the Request Packet should have the #NOT_FOUND status.
 * When the operation code is #FS_GET_NODE_CHILDREN, then the kernel provides a pointer \a *node to the directory VFS node
 * and expects the driver to list all children nodes (files) in given directory, that is, create a linked list of
 * VFS nodes for all files and provide a pointer to the first child using \a *children. If there are no children,
 * it is set to NULL.
*/
union FsGetRequest
{
    /**
     * @brief Get node request body
     */
    struct
    {
        const struct IoVfsNode *parent; /**< Parent to be provided by the caller */
        const char *name; /**< Name of the node to be provided by the caller */
        struct IoVfsNode *node; /**< Requested node returned by the callee */
    } getNode;

    /**
     * @brief Get all node childer request body
     */
    struct
    {
        const struct IoVfsNode *node; /**< Parent to be provided by the caller */
        struct IoVfsNode *children; /**< Linked list of the requested nodes returned by the callee */
    } getChildren;
};


#ifdef __cplusplus
}
#endif

#endif