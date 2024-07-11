#ifndef KERNEL_VFS_H_
#define KERNEL_VFS_H_

#include <stdint.h>
#include <stdbool.h>
#include "defines.h"
#include "ob/ob.h"
#include "io/dev/op.h"
#include "ke/core/mutex.h"

EXPORT
typedef uint32_t IoVfsNodeFlags;
#define IO_VFS_FLAG_READ_ONLY 0x1 //file/directory is read only
#define IO_VFS_FLAG_LOCK 0x2 //node is locked, for example during write - VFS does not check this flag
#define IO_VFS_FLAG_NO_CACHE 0x4 //do not cache this entry
#define IO_VFS_FLAG_VIRTUAL 0x8
#define IO_VFS_FLAG_DIRTY 0x10 //data for this entry changed in cache, must be send to disk first
#define IO_VFS_FLAG_VFS_DIRECTORY 0x10000000
#define IO_VFS_FLAG_PERSISTENT 0x80000000 //persisent entry (unremovable)

EXPORT
typedef uint32_t IoVfsFlags;

EXPORT
#define IO_VFS_FLAG_SYNCHRONOUS 1 //synchronous read/write, that is do no return until completed
#define IO_VFS_FLAG_DIRECT 2 //force unbuffered read/write
#define IO_VFS_FLAG_NO_WAIT 4 //do not wait if file is locked
#define IO_VFS_FLAG_CREATE 8 //create file if doesn't exist

EXPORT
/**
 * @brief VFS node types
*/
enum IoVfsEntryType
{
    IO_VFS_UNKNOWN = 0, //unknown type, for entries not initialized properly that should be ommited
    IO_VFS_MOUNT_POINT, //mount point
    IO_VFS_DEVICE, //device other than disk that provides the IoDeviceObject structure
    IO_VFS_DIRECTORY,
    IO_VFS_FILE,
    IO_VFS_LINK, //symbolic link, might exist physically or not
};

EXPORT
/**
 * @brief VFS filesystem type
*/
enum IoVfsFsType
{
    IO_VFS_FS_UNKNOWN = 0, //unknown filesystem, entry should be ommited
    IO_VFS_FS_PHYSICAL, //filesystem corresponding to a physical filesystem 
    IO_VFS_FS_VIRTUAL,  //virtual FS that can be modified only by the kernel
                        //and does not require any special/separate handling
    IO_VFS_FS_TASKFS, //"/task" virtual filesystem
    IO_VFS_FS_INITRD, //initial ramdisk filesystem
};

EXPORT
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

EXPORT
struct KeTaskControlBlock;

EXPORT
/**
 * @brief Main VFS node structure
*/
struct IoVfsNode
{
    struct ObObjectHeader objectHeader;
    enum IoVfsEntryType type; /**< Node type */
    uint64_t size; /**< Size of underlying data, applies to files only */
    IoVfsNodeFlags flags; /**< Node flags */
    Time_t lastUse; /**< Last node use timestamp */
    KeRwLock lock; /**< Readers-writers lock for the underlying data (file) */
    struct
    {
        uint32_t open : 1; 
        uint32_t scheduledForDeletion : 1; /**< Node scheduled for deletion, treat as non-existing */
    } status;
    

    struct
    {
        uint32_t readers; /**< Number of readers */
        uint32_t writers; /**< Number of writers (0 or 1) */
        uint32_t links; /**< Number of symbolic links to this node */
    } references;

    enum IoVfsFsType fsType; /**< VFS filesystem type this node belongs to */
    struct IoDeviceObject *device; /**< Associated device */
    struct IoVfsNode *linkDestination; /**< Symbolic link destination */
    union IoVfsReference ref; /**< Reference value for the driver (LBA, inode number etc.) */

    struct KeTaskControlBlock *currentTask; /**< Task currently owning the file (writer) */
    struct KeTaskControlBlock *taskQueue; /**< Queue of tasks waiting for access */


    struct IoVfsNode *parent; /**< Higher level (parent) node */
    struct IoVfsNode *child; /**< First lower level (child) node */
    struct IoVfsNode *next; /**< Same level next node in linked list */
    struct IoVfsNode *previous; /**< Same level previous node in linked list */

    char name[]; /**< Node name, length can be obtained using \a IoVfsGetMaxFileNameLength() */
};

EXPORT
/**
 * @brief Get maximum file name length
 * @return Maximum file name length, excluding terminator
*/
EXTERN uint32_t IoVfsGetMaxFileNameLength(void);

/**
 * @brief Initialize Virtual File System and set up core entries
 * @return Status code
*/
STATUS IoVfsInit(void);

/**
 * @brief Detach file name from given path
 * @param *path Input: path to detach the file name from, output: path without file name
 * @return File name pointer
*/
char* IoVfsDetachFileName(char *path);

/**
 * @brief Get VFS node of given name under given parent
 * @param *parent Parent VFS node
 * @param *name Node name
 * @return Requested node or NULL if not found
 * @attention This function does not resolve links
*/
struct IoVfsNode* IoVfsGetNode(struct IoVfsNode *parent, char *name);

/**
 * @brief Get VFS node for given path
 * @param *path Path string
 * @return VFS node or NULL if not found
 * @attention This function resolves links along the path.
 * If the final file is a link, it is not resolved.
*/
struct IoVfsNode* IoVfsGetNodeByPath(char *path);

/**
 * @brief Check if VFS node of given name exists under given parent
 * @param *parent Parent VFS node
 * @param *name Node name
 * @return True if exists, false otherwise
*/
bool IoVfsCheckIfNodeExists(struct IoVfsNode *parent, char *name);

/**
 * @brief Check if VFS with given path exists
 * @param *path Node path
 * @return True if exists, false otherwise
*/
bool IoVfsCheckIfNodeExistsByPath(char *path);

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
 * @warning This function does NOT check if node with given name already exists
*/
void IoVfsInsertNode(struct IoVfsNode *node, struct IoVfsNode *parent);

/**
 * @brief Create symbolic link
 * @param *path Link path with file name
 * @param *linkDestination Link destionation path
 * @param flags Entry flags
 * @return Status code
 * @warning Link destination must exist
*/
STATUS IoVfsCreateLink(char *path, char *linkDestination, IoVfsNodeFlags flags);

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
STATUS IoVfsRead(struct IoVfsNode *node, IoVfsFlags flags, void *buffer, uint64_t size, uint64_t offset, IoReadWriteCompletionCallback callback, void *context);

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
STATUS IoVfsWrite(struct IoVfsNode *node, IoVfsFlags flags, void *buffer, uint64_t size, uint64_t offset, uint64_t *actualSize);

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

EXPORT
/**
 * @brief Lock VFS tree for reading, that is, any operations involving traversing the tree
 */
EXTERN void IoVfsLockTreeForReading(void);

EXPORT
/**
 * @brief Lock VFS tree for writing, that is, any operations involving modyfing the tree
 */
EXTERN void IoVfsLockTreeForWriting(void);

EXPORT
/**
 * @brief Unlock VFS tree
 */
EXTERN void IoVfsUnlockTree(void);

#endif