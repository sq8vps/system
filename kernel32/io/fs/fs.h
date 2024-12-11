#ifndef KERNEL_FS_H_
#define KERNEL_FS_H_

#include <stdint.h>
#include <stdbool.h>
#include "defines.h"
#include "vfs.h"
#include "ob/ob.h"

EXPORT_API

struct KeTaskControlBlock;

/**
 * @brief File operation flags
*/
#define IoFileFlags IoVfsFlags


/**
 * @brief Perform operation directly, omit internal bufferring
*/
#define IO_FILE_FLAG_DIRECT IO_VFS_OPERATION_FLAG_DIRECT
/**
 * @brief Do not wait if file is not available for operation, but fail immediately
*/
#define IO_FILE_FLAG_NO_WAIT IO_VFS_OPERATION_FLAG_NO_WAIT


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
        KeMutex lock; /**< Mutex to ensure thread safety */
        struct KeTaskControlBlock *task; /**< Task that requested the operation */
        size_t actualSize; /**< Actual read/written count of bytes */
        STATUS status; /**< Operation status */
        uint32_t completed : 1; /**< Operation completed */
        uint32_t write : 1; /**< Write/append operation */
        uint64_t offset; /**< Operation offset */
    } operation;

    int id; /**< File descriptor */
    struct IoVfsNode *node; /**< VFS node that this file references to */
    IoFileOpenMode mode; /**< Mode in which the file is open */
    IoFileFlags flags; /**< Additional file flags */
    uint32_t references; /**< Number of references: open + memory mappings */
} IoFileHandle;

/**
 * @brief Open file for given task
 * @param *file File path string
 * @param mode File open mode
 * @param flags File flags
 * @param *handleNumber Output file handle or -1 on failure
 * @return Status code
*/
STATUS IoOpenFile(const char *file, IoFileOpenMode mode, IoFileFlags flags, int *handleNumber);

/**
 * @brief Close file for given task
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
 * @brief Initialize I/O File Manager subsystem
 * @return Status code
*/
INTERNAL STATUS IoFsInit(void);

#endif