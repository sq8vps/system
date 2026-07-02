/**
 * @file vfs.h
 * @brief Virtual File System layer
 * @ingroup io_fs
 */

#ifndef KERNEL_VFS_H_
#define KERNEL_VFS_H_

#include <stdint.h>
#include <stdbool.h>
#include "defines.h"
#include "ob/ob.h"
#include "io/dev/op.h"
#include "ke/core/mutex.h"
#include "fs.h"
#include "taskfs.h"

DRIVER_API

/**
 * @addtogroup io_fs_vfs Virtual File System layer
 * @brief Virtual File System layer
 * @ingroup io_fs
 * 
 * This module is an intermediate layer between the actual file system drivers (such as ext4) and the high-level file support layer,
 * which is used by kernel-mode drivers and user-mode programs to access files.
 * @{
 */

NABLA_API

/**
 * @brief VFS node flags
 */
enum IoVfsFlags
{
    IO_VFS_FLAG_READ_ONLY = 0x1, /**< File is read only */
    IO_VFS_FLAG_HIDDEN = 0x2, /**< Node is hidden from user */
    IO_VFS_FLAG_PERSISTENT = 0x4, /**< Node is unremovable */
    IO_VFS_FLAG_NO_CACHE = 0x8, /**< Do not cache this file */
    IO_VFS_FLAG_NO_HARD_LINKS = 0x10, /**< Parent filesystem doesn't support hard links */
    IO_VFS_FLAG_DIRTY = 0x10000000, /**< File has changed, must be written to disk */
    IO_VFS_FLAG_ATTRIBUTES_DIRTY = 0x20000000, /**< File attributes has changed, must be written to disk */
};

/**
 * @brief VFS node types
*/
enum IoVfsEntryType
{
    IO_VFS_UNKNOWN = 0, /**< Unknown type; for entries not initialized properly that should be ommited */
    IO_VFS_MOUNT_POINT = 1, /**< Mount point */
    IO_VFS_DEVICE = 2, /**< Device other than disk that provides the \ref IoDeviceObject structure */
    IO_VFS_DIRECTORY = 3, /**< Directory */
    IO_VFS_FILE = 4, /**< Regular file */
    IO_VFS_LINK = 5, /**< Symbolic link; either virtual or on a physical device */
};


/**
 * @brief VFS filesystem type
*/
enum IoVfsFsType
{
    IO_VFS_FS_UNKNOWN = 0, /** Unknown filesystem, entry should be ommited */
    IO_VFS_FS_PHYSICAL = 1, /**< Filesystem corresponding to a physical filesystem */
    IO_VFS_FS_VIRTUAL = 2,  /**< Virtual FS that can be modified only by the kernel and does not require any special/separate handling */
    IO_VFS_FS_TASKFS = 3, /**< "/task" virtual filesystem */
    IO_VFS_FS_INITRD = 4, /**< Initial ramdisk filesystem */
};

/**
 * @brief File attributes structure filled by \ref IoGetFileAttributes
 */
struct IoFileAttributes
{
    uint64_t size; /**< File size. For symbolic links, if it resides on a physical filesystem, this is the size of the link file and not the target file. 
    If it resides on a virtual filesystem, it is equal to zero. */
    enum IoVfsFlags flags; /**< File flags */
    enum IoVfsEntryType type; /**< File entry type */
    enum IoVfsFsType fsType; /**< Filesystem type */
    uint32_t links; /**< Number of hard links associated with the file */
    uint32_t uid; /**< User ID */
    uint32_t gid; /**< Group ID */
    uint16_t permissions; /**< File permissions */
    struct TimeSpec accessTime; /**< Last access timestamp */
    struct TimeSpec creationTime; /**< Creation timestamp */
    struct TimeSpec changeTime; /**< Data modification timestamp */
    struct TimeSpec statusChangeTime; /**< Status modification timestamp */
    size_t blockSize; /**< Preferred I/O block size */
    struct
    {
        uint32_t characterDevice : 1; /**< Underlying device is a character device */
    } auxFlags;
};

END_NABLA_API

/**
 * @brief VFS node reference value type
*/
union IoVfsReference
{
    void *p;
    uint64_t u64;
    int64_t i64;
    uint32_t u32;
    int32_t i32;
    uint16_t u16;
    int16_t i16;
    uint8_t u8;
    int8_t i8;
};

/**
 * @brief Main VFS node structure
*/
struct IoVfsNode
{
    OBJECT;
    enum IoVfsEntryType type; /**< Node type */
    uint64_t size; /**< Size of underlying data, applies to files only */
    enum IoVfsFlags flags; /**< Node flags */
    uint32_t gid; /**< Group ID */
    uint32_t uid; /**< User ID */
    uint16_t permissions; /**< File permissions */
    struct TimeSpec lastAccessTime; /**< Last node use timestamp */
    struct TimeSpec creationTime; /**< File creation timestamp */
    struct TimeSpec lastChangeTime; /**< File modification timestamp */
    struct TimeSpec lastStatusChangeTime; /**< File status modification timestamp */
    KeRwLock lock; /**< Readers-writers lock for the underlying data (file) and attributes */
    struct
    {
        uint32_t open : 1; /**< File is open */
    } status;
    
    struct
    {
        uint32_t readers; /**< Number of readers */
        uint32_t writers; /**< Number of writers */
        uint32_t links; /**< Number of hard links to the underlying data */
    } references;

    struct IoTaskFsContext taskfs; /**< Special data for /taskfs filesystem */

    enum IoVfsFsType fsType; /**< VFS filesystem type this node belongs to */
    struct IoDeviceObject *device; /**< Associated device */
    struct IoVfsNode *linkDestination; /**< Symbolic link destination */
    union IoVfsReference ref[2]; /**< Reference value for the driver (LBA, inode number etc.) */

    struct IoVfsNode *parent; /**< Higher level (parent) node */
    struct IoVfsNode *child; /**< First lower level (child) node */
    struct IoVfsNode *next; /**< Same level next node in linked list */
    struct IoVfsNode *previous; /**< Same level previous node in linked list */

    char name[]; /**< Node name, length can be obtained using \a IoVfsGetMaxFileNameLength() */
};


/**
 * @brief Get maximum file name length
 * @return Maximum file name length, excluding terminator
*/
uint32_t IoVfsGetMaxFileNameLength(void);

/**
 * @brief Initialize Virtual File System and set up core entries
 * @return Status code
*/
STATUS IoVfsInit(void);

/**
 * @brief Get VFS node for given path, optionally excluding the last path element
 * @param *path Path string
 * @param excludeLastElement True to exclude last path element
 * @param *taskfs Task file system context
 * @return VFS node or NULL if not found
 * @attention This function resolves links along the path.
 * If the final file is a link, it is not resolved.
*/
struct IoVfsNode *IoVfsGetNodeEx(const char *path, bool excludeLastElement, struct IoTaskFsContext *taskfs);

/**
 * @brief Get VFS node for given path
 * @param path Path string
 * @param taskfs Task filesystem context
 * @return VFS node or NULL if not found
 * @attention This function resolves links along the path.
 * If the final file is a link, it is not resolved.
*/
#define IoVfsGetNode(path, taskfs) IoVfsGetNodeEx(path, false, taskfs)

/**
 * @brief Check if VFS node with given path exists
 * @param *path Node path
 * @return True if exists, false otherwise
*/
bool IoVfsCheckIfNodeExists(const char *path);

/**
 * @brief Check if VFS node with given name exists under a given parent
 * @param *parent Parent VFS node
 * @param *name Node name
 * @return True if exists, false otherwise
*/
bool IoVfsCheckIfNodeExistsByParent(struct IoVfsNode *parent, const char *name);

/**
 * @brief Insert previously prepared node at given path
 * @param *node VFS node to insert
 * @param *path Path to insert the file to, either including the target file name or not
 * @param isFilePath Set to true if \a *path is a file path rather than parent directory path
 * @return Status code
 * @warning This function does NOT check if node with given name already exists
*/
STATUS IoVfsInsertNodeByPath(struct IoVfsNode *node, const char *path, bool isFilePath);

/**
 * @brief Insert previously prepared node at given parent directory path
 * @param *node VFS node to insert
 * @param *path Parent directory path
 * @return Status code
 * @warning This function does NOT check if node with given name already exists
*/
#define IoVfsInsertNodeByParentPath(node, path) IoVfsInsertNodeByPath(node, path, false)

/**
 * @brief Insert previously prepared node at given position
 * @param *node VFS node to insert
 * @param *path Target file path. Node name and path file name must match.
 * @return Status code
 * @warning This function does NOT check if node with given name already exists
*/
#define IoVfsInsertNodeByFilePath(node, path) IoVfsInsertNodeByPath(node, path, true)

/**
 * @brief Insert previously prepared node under given parent node
 * @param *node VFS node to insert
 * @param *parent Parent node
 * @warning This function does NOT check if node with given name already exists
*/
void IoVfsInsertNode(struct IoVfsNode *node, struct IoVfsNode *parent);

/**
 * @brief Open file described by the VFS node
 * @param *node VFS node
 * @param write True to open for writing, false to open for reading only
 * @param flags Additional flags
 * @return Status code
 */
STATUS IoVfsOpen(struct IoVfsNode *node, bool write, IoFileFlags flags);

/**
 * @brief Close file described by the VFS node
 * @param *node VFS node
 * @return Status code
 */
STATUS IoVfsClose(struct IoVfsNode *node);

/**
 * @brief Create a new file
 * @param *path Full path to the new file
 * @param type Type of the file
 * @param flags VFS flags
 * @param **node Output VFS node
 * @return Status code
 */
STATUS IoVfsCreate(const char *path, enum IoVfsEntryType type, enum IoVfsFlags flags, struct IoVfsNode **node);

/**
 * @brief Resolve link
 * @param *node Link node
 * @param *taskfs Task file system context
 * @return Resolved link node - the target file that this link points to
 * @note If \a *node is not a link, then it is returned immediately
 */
struct IoVfsNode *IoVfsResolveLink(struct IoVfsNode *node, struct IoTaskFsContext *taskfs);

/**
 * @brief Create symbolic link
 * @param *path Link path
 * @param *destination Link destination path
 * @param flags VFS node flags
 * @return Status code
 * @warning Link destination must exist
*/
STATUS IoVfsCreateSymLink(const char *path, const char *destination, enum IoVfsFlags flags);

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
 * @brief Read file
 * @param *node VFS node containing the file
 * @param flags File operation flags
 * @param *buffer Destination buffer
 * @param size Count of bytes to read (at most)
 * @param offset Offset into the file in bytes
 * @param callback Operation completion callback
 * @param *context Callback context
 * @return Status code
*/
STATUS IoVfsRead(struct IoVfsNode *node, IoFileFlags flags, void *buffer, size_t size, uint64_t offset, IoReadWriteCompletionCallback callback, void *context);

/**
 * @brief Write file
 * @param *node VFS node containing the file
 * @param flags File operation flags
 * @param *buffer Destination buffer
 * @param size Count of bytes to write
 * @param offset Offset into the file in bytes
 * @param callback Operation completion callback
 * @param *context Callback context
 * @return Status code
*/
STATUS IoVfsWrite(struct IoVfsNode *node, IoFileFlags flags, void *buffer, size_t size, uint64_t offset, IoReadWriteCompletionCallback callback, void *context);


/**
 * @brief Create VFS node with given name
 * @param *name Node name
 * @return Node pointer or NULL on failure
*/
struct IoVfsNode* IoVfsCreateNode(const char *name);


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
STATUS IoVfsGetSize(const char *path, uint64_t *size);

/**
 * @brief Get file attributes/status
 * @param fd File handle. If -1, then \a path is used.
 * @param *path Path of the file. Used if \a fd is non-negative.
 * @param dontResolveLink If true and the target file is a link, then it won't be resolved
 * @param *attr Pointer to where store the attributes.
 * @return Status code
 */
STATUS IoGetFileAttributes(int fd, const char *path, bool dontResolveLink, struct IoFileAttributes *attr);


/**
 * @brief Lock VFS tree for reading, that is, any operations involving traversing the tree
 */
void IoVfsLockTreeForReading(void);


/**
 * @brief Lock VFS tree for writing, that is, any operations involving modyfing the tree
 */
void IoVfsLockTreeForWriting(void);


/**
 * @brief Unlock VFS tree
 */
void IoVfsUnlockTree(void);

NABLA_API

/**
 * @brief Get file attributes/status
 * @param fd File handle. If -1, then \a path is used.
 * @param *path Path of the file. Used if \a fd is non-negative.
 * @param dontResolveLink If true and the target file is a link, then it won't be resolved
 * @param *attr Pointer to where store the attributes.
 * @return Status code
 */
STATUS ApiGetFileAttributes(int fd, const char *path, bool dontResolveLink, struct IoFileAttributes *attr);

END_NABLA_API


END_DRIVER_API

/**
 * @}
 */

#endif