/**
 * @file fs.h
 * @brief File system DDK and helpers
 * @ingroup ddk
 */

#ifndef DDK_FS_H_
#define DDK_FS_H_

#include <stdint.h>
#include <stdbool.h>
#include "defines.h"

DRIVER_API

#include "io/fs/vfs.h"

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
    FS_CREATE = 3, /**< Create file or directory */
    FS_RENAME = 4, /**< Rename file or directory */
    FS_LINK = 5, /**< Create a hard link */
    FS_UNLINK = 6, /**< Remove a hard link */
    FS_REMOVE = FS_UNLINK, /**< Remove file or directory. This is the same as \ref FS_UNLINK */
};


union FsRequest
{
    /**
     * @brief Get node request body
     * 
     * This structure is used with \ref FS_GET_NODE. The kernel provides a pointer \a *parent to the parent VFS node
     * and a name of the node to be found. The driver then allocates a VFS node for the file/directory and returns
     * it through \a *node (it does NOT attach this node to the tree!). If such node does not exist, then NULL is returned
     * and the Request Packet should have the #NOT_FOUND status.
     */
    struct
    {
        const struct IoVfsNode *parent; /**< Parent to be provided by the caller */
        const char *name; /**< Name of the node to be provided by the caller */
        struct IoVfsNode *node; /**< Requested node returned by the callee */
    } getNode;

    /**
     * @brief Get all node childer request body
     * 
     * This structure is used with \ref FS_GET_CHILDREN. The kernel provides a pointer \a *node to the directory VFS node
     * and expects the driver to list all children nodes (files) in given directory, that is, create a linked list of
     * VFS nodes for all files and provide a pointer to the first child using \a *children. If there are no children,
     * it is set to NULL.
     */
    struct
    {
        const struct IoVfsNode *node; /**< Parent to be provided by the caller */
        struct IoVfsNode *children; /**< Linked list of the requested nodes returned by the callee */
    } getChildren;

    /** 
     * @brief Create file request body
     * 
     * 
     * This structure is used with \ref FS_CREATE.
     * The kernel sets \a *parent to the parent node for the newly created file, provides \a *name and \a type of the new file,
     * and also some \a flags. The VFS node of the newly created file is returned in \a *node.
     */
    struct
    {
        const struct IoVfsNode *parent; /**< Parent to be provided by the caller */
        const char *name; /**< Name of the file to be created */
        enum IoVfsEntryType type; /**< Type of the file to be created */
        enum IoVfsFlags flags; /**< Additional flags for the file */
        struct IoVfsNode *node; /**< VFS node of the created file, returned by the callee */
    } create;

    /** 
     * @brief Rename file request body
     * 
     * 
     * This structure is used with \ref FS_RENAME.
     * The kernel sets \a *node to the node which name is to be changed, and provides a new \a *name.
     */
    struct
    {
        struct IoVfsNode *node; /**< Target node */
        const char *name; /**< New name of the file */
    } rename;

    /**
     * @brief Create hard link request body
     * @note Some file systems do not support hard links
     *
     * This structure is used with \ref FS_LINK.
     * The kernel sets \a *node to the target node, and provides a \a name for the hard link.
     */
    struct
    {
        struct IoVfsNode *node; /**< Target node */
        const char *name; /**< Name of the new hard link */
    } link;
    
    /**
     * @brief Remove hard link/remove file or directory request body
     * 
     * This structure is used with \ref FS_UNLINK or \ref FS_REMOVE (which is the same).
     * The kernel sets \a *node to the target node that needs to be removed.
     */
    struct
    {
        struct IoVfsNode *node; /**< Target node */
    } unlink, remove;
};


END_DRIVER_API

/**
 * @brief Get VFS node with given name and parent from the filesystem
 * @param *parent Parent VFS node
 * @param *name Node name
 * @param **node Output node or NULL if not found (check return code)
 * @return Status code
*/
INTERNAL STATUS FsGetNode(const struct IoVfsNode *parent, const char *name, struct IoVfsNode **node);

/**
 * @brief Get VFS node children list
 * @param *node VFS node to find the children of
 * @param **children Output children list or NULL if no children (check return code)
 * @return Status code
*/
INTERNAL STATUS FsGetNodeChildren(const struct IoVfsNode *node, struct IoVfsNode **children);

/**
 * @brief Create file on a filysystem
 * @param *parent Parent VFS node for the new file
 * @param *name Name of the new file
 * @param type VFS type of the new file
 * @param flags VFS flags for the new file
 * @param **node Output node of the created file
 * @return Status code
 */
INTERNAL STATUS FsCreateFile(const struct IoVfsNode *parent, const char *name, enum IoVfsEntryType type, enum IoVfsFlags flags, struct IoVfsNode **node);

/**
 * @}
 */

#endif