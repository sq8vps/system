#ifndef KERNEL_FS_H_
#define KERNEL_FS_H_

#include <stdint.h>
#include <stdbool.h>
#include "defines.h"
#include "ob/ob.h"
#include "io/dev/op.h"
#include "taskfs.h"

EXPORT_API

struct KeTaskControlBlock;
struct IoVfsNode;

/**
 * @brief File flags
*/
typedef enum
{
    IO_FILE_FLAG_DIRECT = 0x1, /**< Force performing operation directly (omitting internal bufferring), fail if not possible */
    IO_FILE_FLAG_NO_WAIT = 0x2, /**< Do not wait if file is not available for operation, but fail immediately */
    IO_FILE_FLAG_SHARED = 0x4, /**< Allow other processes to use the file regardless of mode and locking policy */
    IO_FILE_FLAG_FORCE_HANDLE_NUMBER = 0x8, /**< Force the provided handle number to be used, fail if not possible */
    IO_FILE_NO_LINK_RESOLUTION = 0x10, /**< If the target file is a link, do not resolve it, but rather work on the link itself */
} IoFileFlags;


/**
 * @brief File open modes
 * 
 * File content is available for reading in \a IO_FILE_READ, \a IO_FILE_WRITE and \a IO_FILE_APPEND modes.
 * \a IO_FILE_WRITE allows to write to any offset without destroying current file content (except the content being explicitly overwritten).
 * \a IO_FILE_APPEND allows to append content to the end and ignores provided offset.
 * \a IO_FILE_REPLACE flag can be ORed with open mode to destroy current file content.
 * \a IO_FILE_READ_ATTRIBUTES allows to read file attributes via \a IoFileAttributes structure.
 * \a IO_FILE_WRITE_ATTRIBUTES allows to write file attributes via \a IoFileAttributes structure.
 * By default, when the file does not exist, the function fails and nothing is read nor written.
 * \a IO_FILE_CREATE flag ORed with open mode allows to create the file if it does not exist.
 * Using \a IO_FILE_READ_ATTRIBUTES or \a IO_FILE_WRITE_ATTRIBUTES with \a IO_FILE_WRITE 
 * or \a IO_FILE_APPEND is illegal and result with open error.
*/
typedef enum
{
    IO_FILE_READ = 0x0, /**< Open file for reading */
    IO_FILE_WRITE = 0x1, /**< Open file for writing (to any offset) */
    IO_FILE_APPEND = 0x2, /**< Open file for appending (offset ignored) */
    IO_FILE_READ_ATTRIBUTES = 0x4, /**< Open file for reading attributes */
    IO_FILE_WRITE_ATTRIBUTES = 0x8, /**< Open file for writing attributes */
    IO_FILE_CREATE = 0x10, /**< Create file if does not exist */
    IO_FILE_REPLACE = 0x20, /**< Replace file content on writing (destroy old content) */
} IoFileOpenMode;


/**
 * @brief Kernel file handle
*/
typedef struct IoFileHandle
{
    OBJECT;
    struct
    {
        KeSpinlock lock; /**< \a operation structure lock */
        struct KeTaskControlBlock *task; /**< Task that requested the operation */
        size_t actualSize; /**< Actual read/written count of bytes */
        STATUS status; /**< Operation status */
        uint32_t completed : 1; /**< Operation completed */
        uint32_t write : 1; /**< Write/append operation */
        uint64_t offset; /**< Operation offset */
    } operation;

    int id; /**< File descriptor */
    struct IoVfsNode *node; /**< VFS node that this file references to */
    struct IoTaskFsContext taskfs; /**< Task file system context if this file references /taskfs */
    IoFileOpenMode mode; /**< Mode in which the file is open */
    IoFileFlags flags; /**< Additional file flags */
    uint32_t references; /**< Number of references: open + memory mappings */
} IoFileHandle;

/**
 * @brief Open file
 * @param *file File path string
 * @param mode File open mode
 * @param flags File flags
 * @param *handleNumber Output file handle or -1 on failure
 * @return Status code
*/
STATUS IoOpenFile(const char *file, IoFileOpenMode mode, IoFileFlags flags, int *handleNumber);

/**
 * @brief Close file
 * @param handleNumber File handle
 * @return Status code
*/
STATUS IoCloseFile(int handleNumber);

/**
 * @brief Read file asynchronously
 * @param handle File handle
 * @param *buffer Destination buffer
 * @param size Count of bytes to read (max size)
 * @param offset Offset into the file in bytes
 * @param callback Callback function on read completion
 * @param *context Context to be passed to the callback function
 * @return Status code
*/
STATUS IoReadFile(int handle, void *buffer, size_t size, uint64_t offset, 
    IoReadWriteCompletionCallback callback, void *context);

/**
 * @brief Write file asynchronously
 * @param handle File handle
 * @param *buffer Source buffer
 * @param size Count of bytes to write
 * @param offset Offset into the file in bytes
 * @param callback Callback function on write completion
 * @param *context Context to be passed to the callback function
 * @return Status code
*/
STATUS IoWriteFile(int handle, void *buffer, size_t size, uint64_t offset,
    IoReadWriteCompletionCallback callback, void *context);


/**
 * @brief Read file synchronously
 * @param handle File handle
 * @param *buffer Source buffer
 * @param size Count of bytes to write
 * @param offset Offset into the file in bytes
 * @param *actualSize Count of bytes actually read
 * @return Status code
*/
STATUS IoReadFileSync(int handle, void *buffer, size_t size, uint64_t offset, size_t *actualSize);


/**
 * @brief Write file synchronously
 * @param handle File handle
 * @param *buffer Source buffer
 * @param size Count of bytes to write
 * @param offset Offset into the file in bytes
 * @param *actualSize Count of bytes actually written
 * @return Status code
*/
STATUS IoWriteFileSync(int handle, void *buffer, size_t size, uint64_t offset, size_t *actualSize);


/**
 * @brief Check if file exists
 * @param *file File path
 * @return True if exists, false if not
*/
bool IoCheckIfFileExists(const char *file);


/**
 * @brief Get file size
 * @param *file File path
 * @param *size Pointer to where to store size
 * @return Status code
*/
STATUS IoGetFileSize(const char *file, uint64_t *size);

END_EXPORT_API

/**
 * @brief Open file for given process
 * @param *pcb Process Control Block
 * @param *file File path string, used if \a fileNode is NULL
 * @param *fileNode VFS node, used instead of \a file if not NULL
 * @param *taskfs Task file system context
 * @param mode File open mode
 * @param flags File flags
 * @param *handleNumber Output file handle or -1 on failure
 * @return Status code
*/
INTERNAL STATUS IoOpenFileForProcess(struct KeProcessControlBlock *pcb, const char *file, struct IoVfsNode *fileNode, struct IoTaskFsContext *taskfs, IoFileOpenMode mode, IoFileFlags flags, int *handleNumber);

/**
 * @brief Close file for given process
 * @param *pcb Process Control Block
 * @param handleNumber File handle
 * @return Status code
*/
INTERNAL STATUS IoCloseFileForProcess(struct KeProcessControlBlock *pcb, int handleNumber);

/**
 * @brief Clone file handles to the new process from the calling process
 * @param *pcb Target Process Control Block
 * @param targetHandle Handle number in target process
 * @param sourceHandle Handle number in calling process
 * @return Status code
 */
INTERNAL STATUS IoCloneFileToNewProcess(struct KeProcessControlBlock *pcb, int targetHandle, int sourceHandle);

/**
 * @brief Get VFS node associated with given file handle from given process
 * @param *pcb Owner Process Control Block
 * @param handle File handle number
 * @return Associated VFS node or NULL on failure
 */
INTERNAL struct IoVfsNode* IoGetVfsNodeForFile(struct KeProcessControlBlock *pcb, int handle);

/**
 * @brief Initialize I/O File Manager subsystem
 * @return Status code
*/
INTERNAL STATUS IoFsInit(void);

#endif