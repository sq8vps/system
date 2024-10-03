//This header file is generated automatically
#ifndef EXPORTED___API__IO_FS_FS_H_
#define EXPORTED___API__IO_FS_FS_H_

#ifdef __cplusplus
extern "C" 
{
#endif

#include <stdint.h>
#include <stdbool.h>
#include "defines.h"
#include "vfs.h"
#include "ob/ob.h"

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
 * \a IO_FILE_WRITE allows to write to any offset without destroying current file content.
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
 * @brief File handle
*/
typedef struct IoFileHandle
{
    struct ObObjectHeader objectHeader;
    bool free; /**< Does this entry represent free handle range? */
    struct KeTaskControlBlock *task; /**< Task that this file belongs to */
    struct
    {
        uint64_t actualSize; /**< Actual read/written count of bytes */
        STATUS status; /**< Operation status */
        uint32_t completed : 1; /**< Operation completed */
        uint32_t write : 1; /**< Write/append operation */
        uint64_t offset; /**< Operation offset */
    } operation;
    union
    {
        struct
        {
            int id; /**< File descriptor */
            struct IoVfsNode *node; /**< VFS node that this file references to */
            IoFileOpenMode mode; /**< Mode in which the file is open */
            IoFileFlags flags; /**< Additional file flags */
        } fileHandle;
        struct
        {
            int first; /**< First free handle */
            int last; /**< Last free handle */
        } freeHandleRange;
    } type;
    
    struct IoFileHandle *next; /**< Next file handle with consecutive ID */
    struct IoFileHandle *previous; /**< Previous file handle */
} IoFileHandle;

/**
 * @brief Open file for given task
 * @param *file File path string
 * @param mode File open mode
 * @param flags File flags
 * @param *task Task Control Block
 * @param *handleNumber Output file handle or -1 on failure
 * @return Status code
*/
STATUS IoOpenFile(char *file, IoFileOpenMode mode, IoFileFlags flags, struct KeTaskControlBlock *task, int *handleNumber);

/**
 * @brief Close file for given task
 * @param *task Task Control Block
 * @param handleNumber File handle
 * @return Status code
*/
STATUS IoCloseFile(struct KeTaskControlBlock *task, int handleNumber);


/**
 * @brief Open file in kernel mode
 * @param *file File path string
 * @param mode File open mode
 * @param flags File flags
 * @param **handle Output file handle
 * @return Status code
*/
STATUS IoOpenKernelFile(const char *file, IoFileOpenMode mode, IoFileFlags flags, struct IoFileHandle **handle);


/**
 * @brief Close file in kernel mode
 * @param *handle File handle
 * @return Status code
*/
STATUS IoCloseKernelFile(struct IoFileHandle *handle);


/**
 * @brief Read file asynchronously
 * @param *task Task Control Block
 * @param handle File handle
 * @param *buffer Destination buffer
 * @param size Count of bytes to read (max size)
 * @param offset Offset into the file in bytes
 * @param callback Callback function on read completion
 * @param *context Context to be passed to the callback function
 * @return Status code
*/
STATUS IoReadFile(struct KeTaskControlBlock *task, int handle, void *buffer, uint64_t size, uint64_t offset, 
    IoReadWriteCompletionCallback callback, void *context);


/**
 * @brief Read file in kernel mode (globally) asynchronously
 * @param *handle File handle
 * @param *buffer Destination buffer
 * @param size Count of bytes to read (max size)
 * @param offset Offset into the file in bytes
 * @param callback Callback function on read completion
 * @param *context Context to be passed to the callback function
 * @return Status code
*/
STATUS IoReadKernelFile(struct IoFileHandle *handle, void *buffer, uint64_t size, uint64_t offset,
    IoReadWriteCompletionCallback callback, void *context);


/**
 * @brief Write file asynchronously
 * @param *task Task Control Block
 * @param handle File handle
 * @param *buffer Source buffer
 * @param size Count of bytes to write
 * @param offset Offset into the file in bytes
 * @param callback Callback function on write completion
 * @param *context Context to be passed to the callback function
 * @return Status code
*/
STATUS IoWriteFile(struct KeTaskControlBlock *task, int handle, void *buffer, uint64_t size, uint64_t offset,
    IoReadWriteCompletionCallback callback, void *context);


/**
 * @brief Write file in kernel mode (globally) asynchronously
 * @param *handle File handle
 * @param *buffer Source buffer
 * @param size Count of bytes to write
 * @param offset Offset into the file in bytes
 * @param callback Callback function on write completion
 * @param *context Context to be passed to the callback function
 * @return Status code
*/
STATUS IoWriteKernelFile(struct IoFileHandle *handle, void *buffer, uint64_t size, uint64_t offset, 
    IoReadWriteCompletionCallback callback, void *context);


/**
 * @brief Read or write file in kernel mode (globally) synchronously
 * @param *handle File handle
 * @param *buffer Source buffer
 * @param size Count of bytes to write
 * @param offset Offset into the file in bytes
 * @param *actualSize Count of bytes actually read or written
 * @param write True for writing, false for reading
 * @return Status code
*/
STATUS IoReadWriteKernelFileSync(struct IoFileHandle *handle, void *buffer, uint64_t size, uint64_t offset, 
    uint64_t *actualSize, bool write);


/**
 * @brief Read or write file synchronously
 * @param *task Task Control Block
 * @param handle File handle
 * @param *buffer Source buffer
 * @param size Count of bytes to write
 * @param offset Offset into the file in bytes
 * @param *actualSize Count of bytes actually read or written
 * @param write True for writing, false for reading
 * @return Status code
*/
STATUS IoReadWriteFileSync(struct KeTaskControlBlock *task, int handle, void *buffer, uint64_t size, uint64_t offset, 
    uint64_t *actualSize, bool write);


/**
 * @brief Read file in kernel mode (globally) synchronously
 * @param *handle File handle
 * @param *buffer Source buffer
 * @param size Count of bytes to write
 * @param offset Offset into the file in bytes
 * @param *actualSize Count of bytes actually read
 * @return Status code
*/
STATUS IoReadKernelFileSync(struct IoFileHandle *handle, void *buffer, uint64_t size, uint64_t offset, uint64_t *actualSize);


/**
 * @brief Write file in kernel mode (globally) synchronously
 * @param *handle File handle
 * @param *buffer Source buffer
 * @param size Count of bytes to write
 * @param offset Offset into the file in bytes
 * @param *actualSize Count of bytes actually written
 * @return Status code
*/
STATUS IoWriteKernelFileSync(struct IoFileHandle *handle, void *buffer, uint64_t size, uint64_t offset, uint64_t *actualSize);


/**
 * @brief Read file synchronously
 * @param *task Task Control Block
 * @param handle File handle
 * @param *buffer Source buffer
 * @param size Count of bytes to write
 * @param offset Offset into the file in bytes
 * @param *actualSize Count of bytes actually read
 * @return Status code
*/
STATUS IoReadFileSync(struct KeTaskControlBlock *task, int handle, void *buffer, uint64_t size, uint64_t offset, uint64_t *actualSize);


/**
 * @brief Write file synchronously
 * @param *task Task Control Block
 * @param handle File handle
 * @param *buffer Source buffer
 * @param size Count of bytes to write
 * @param offset Offset into the file in bytes
 * @param *actualSize Count of bytes actually written
 * @return Status code
*/
STATUS IoWriteFileSync(struct KeTaskControlBlock *task, int handle, void *buffer, uint64_t size, uint64_t offset, uint64_t *actualSize);


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


#ifdef __cplusplus
}
#endif

#endif