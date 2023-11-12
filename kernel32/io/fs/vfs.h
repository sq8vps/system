#ifndef KERNEL_VFS_H_
#define KERNEL_VFS_H_

#include <stdint.h>
#include <stdbool.h>
#include "defines.h"
#include "ke/core/mutex.h"
#include "fstypedefs.h"

#define IO_VFS_MAX_LEVELS 256 //maximum directory levels in filesystem
#define IO_VFS_MAX_FILE_NAME_LENGTH 1024 //maximum file name length
#define IO_VFS_MAX_PATH_LENGTH (IO_VFS_MAX_LEVELS * (IO_VFS_MAX_FILE_NAME_LENGTH + 1)) //maximum path length

/**
 * @brief VFS node types
*/
enum IoVfsEntryType
{
    IO_VFS_UNKNOWN = 0, //unknown type, for entries not initialized properly that should be ommited
    IO_VFS_DISK, //disk (block device)
    IO_VFS_DEVICE, //device other than disk that provides the IoDeviceObject structure
    IO_VFS_DIRECTORY,
    IO_VFS_FILE,
    IO_VFS_LINK, //symbolic link, might exist physically or not
};

/**
 * @brief Major VFS filesystem type
*/
enum IoVfsMajorType
{
    IO_VFS_FS_UNKNOWN = 0, //unknown major filesystem, entry should be ommited
    IO_VFS_FS_PHYSICAL, //filesystem corresponding to a physical filesystem 
    IO_VFS_FS_VIRTUAL,  //virtual FS that can be modified only by the kernel
                        //and does not require any special/separate handling
    IO_VFS_FS_TASKFS, //"/task" virtual filesystem
    IO_VFS_FS_INITRD, //initial ramdisk filesystem
};

/**
 * @brief Main VFS node structure
*/
struct IoVfsNode
{
    char *name; /**< Node name */
    enum IoVfsEntryType type; /**< Node type */
    uint64_t size; /**< Size of underlying data, applies to files only */
    IoVfsFlags flags; /**< Node flags */
    Time_t lastUse; /**< Last node use timestamp */
    uint32_t referenceCount; /**< Count of references to this object, including symlinks */

    enum IoVfsMajorType majorType; /**< Major VFS filesystem type this node belongs to */
    struct IoDeviceObject *device; /**< Associated device */
    struct IoVfsNode *linkDestination; /**< Symbolic link destination */
    uint64_t referenceValue; /**< Reference value for the driver (LBA, inode number etc.) */

    struct IoVfsNode *parent; /**< Higher level (parent) node */
    struct IoVfsNode *child; /**< First lower level (child) node */
    struct IoVfsNode *next; /**< Same level next node in linked list */
    struct IoVfsNode *previous; /**< Same level previous node in linked list */
};

/**
 * @brief Initialize Virtual File System and set up core entries
 * @return Status code
*/
STATUS IoVfsInit(void);

/**
 * @brief Get VFS node for given path
 * @param *path Path string
 * @return VFS node or NULL if not found
*/
struct IoVfsNode* IoVfsGetNode(char *path);

/**
 * @brief Insert previously prepared node at given position in the tree
 * @param *node VFS node to insert
 * @param *path Parent directory path to insert the node to
 * @return Status code
 * @warning This function does NOT check if node with given name already exists
*/
STATUS IoVfsInsertNodeByPath(struct IoVfsNode *node, char *path);

/**
 * @brief Insert previously prepared node under given parent node
 * @param *node VFS node to insert
 * @param *parent Parent node
 * @return \a *node or NULL on failure
 * @warning This function does NOT check if node with given name already exists
*/
struct IoVfsNode* IoVfsInsertNode(struct IoVfsNode *node, struct IoVfsNode *parent);

/**
 * @brief Create symbolic link
 * @param *path Link path with file name
 * @param *linkDestination Link destionation path
 * @param flags Entry flags
 * @return Status code
 * @warning Link destination must exist
*/
STATUS IoVfsCreateLink(char *path, char *linkDestination, IoVfsFlags flags);

/**
 * @brief Remove symbolic link
 * @param *path Link path with file name
 * @return Status code
*/
STATUS IoVfsRemoveLink(char *path);

/**
 * @brief Remove (detach) VFS node
 * @param *node Node pointer
 * @return Status code
 * @warning This function fails on deletion of node with children or with non-zero reference count.
 * @attention This function does not actually remove the node from the memory
*/
STATUS IoVfsRemoveNode(struct IoVfsNode *node);

/**
 * @brief Read given file
 * @param *node VFS node containing the file
 * @param flags File operation flags
 * @param *buffer Destination buffer
 * @param size Count of bytes to read (at most)
 * @param offset Offset into the file in bytes
 * @param *actualSize Count of bytes actually read
 * @return Status code
*/
STATUS IoVfsRead(struct IoVfsNode *node, IoVfsOperationFlags flags, void *buffer, uint64_t size, uint64_t offset, uint64_t *actualSize);

/**
 * @brief Write to given file
 * @param *node VFS node containing the file
 * @param flags File operation flags
 * @param *buffer Source buffer
 * @param size Count of bytes to write
 * @param offset Offset into the file in bytes
 * @param *actualSize Count of bytes actually written
 * @return Status code
*/
STATUS IoVfsWrite(struct IoVfsNode *node, IoVfsOperationFlags flags, void *buffer, uint64_t size, uint64_t offset, uint64_t *actualSize);

/**
 * @brief Create VFS node with given name
 * @param *name Node name
 * @return Node pointer or NULL on failure
*/
struct IoVfsNode* IoVfsCreateNode(char *name);

/**
 * @brief Destroy (deallocate) VFS node
 * @param *node Node to destroy
*/
void IoVfsDestroyNode(struct IoVfsNode *node);

/**
 * @brief Get size of the element
 * @param *path Path to the element
 * @param *size Pointer to where to store the size
 * @return Status code
*/
STATUS IoVfsGetSize(char *path, uint64_t *size);

#endif