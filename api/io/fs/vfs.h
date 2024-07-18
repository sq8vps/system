//This header file is generated automatically
#ifndef EXPORTED___API__IO_FS_VFS_H_
#define EXPORTED___API__IO_FS_VFS_H_

#ifdef __cplusplus
extern "C" 
{
#endif

#include <stdint.h>
#include <stdbool.h>
#include "defines.h"
#include "ob/ob.h"
#include "io/dev/op.h"
#include "ke/core/mutex.h"
typedef uint32_t IoVfsNodeFlags;
#define IO_VFS_FLAG_READ_ONLY 0x1 //file/directory is read only
#define IO_VFS_FLAG_NO_CACHE 0x4 //do not cache this entry
#define IO_VFS_FLAG_DIRTY 0x10 //data for this entry changed in cache, must be send to disk first
#define IO_VFS_FLAG_ATTRIBUTES_DIRTY 0x20
#define IO_VFS_FLAG_HIDDEN 0x40 /**< Node should be hidden from normal user */
#define IO_VFS_FLAG_PERSISTENT 0x80000000 //persisent entry (unremovable)

typedef uint32_t IoVfsFlags;

#define IO_VFS_FLAG_SYNCHRONOUS 1 //synchronous read/write, that is do no return until completed
#define IO_VFS_FLAG_DIRECT 2 //force unbuffered read/write
#define IO_VFS_FLAG_NO_WAIT 4 //do not wait if file is locked
#define IO_VFS_FLAG_CREATE 8 //create file if doesn't exist

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
    struct ObObjectHeader objectHeader;
    enum IoVfsEntryType type; /**< Node type */
    uint64_t size; /**< Size of underlying data, applies to files only */
    IoVfsNodeFlags flags; /**< Node flags */
    time_t lastUse; /**< Last node use timestamp */
    time_t creationTime; /**< File creation timestamp */
    time_t lastModification; /**< File modification timestamp */
    KeRwLock lock; /**< Readers-writers lock for the underlying data (file) and attributes */
    struct
    {
        uint32_t open : 1; /**< File is open */
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
extern uint32_t IoVfsGetMaxFileNameLength(void);

/**
 * @brief Create VFS node with given name
 * @param *name Node name
 * @return Node pointer or NULL on failure
*/
extern struct IoVfsNode* IoVfsCreateNode(char *name);

/**
 * @brief Destroy (deallocate) VFS node
 * @param *node Node to destroy
*/
extern void IoVfsDestroyNode(struct IoVfsNode *node);

/**
 * @brief Lock VFS tree for reading, that is, any operations involving traversing the tree
 */
extern void IoVfsLockTreeForReading(void);

/**
 * @brief Lock VFS tree for writing, that is, any operations involving modyfing the tree
 */
extern void IoVfsLockTreeForWriting(void);

/**
 * @brief Unlock VFS tree
 */
extern void IoVfsUnlockTree(void);


#ifdef __cplusplus
}
#endif

#endif