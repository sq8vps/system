#ifndef KERNEL_FS_H_
#define KERNEL_FS_H_

#include <stdint.h>
#include <stdbool.h>
#include "defines.h"
#include "vfs.h"
#include "ob/ob.h"

EXPORT
struct KeTaskControlBlock;


EXPORT
/**
 * @brief File operation flags
*/
#define IoFileFlags IoVfsFlags

EXPORT
/**
 * @brief Use synchronous (blocking) operations
*/
#define IO_FILE_FLAG_SYNCHRONOUS IO_VFS_OPERATION_FLAG_SYNCHRONOUS
/**
 * @brief Perform operation directly, omit internal bufferring
*/
#define IO_FILE_FLAG_DIRECT IO_VFS_OPERATION_FLAG_DIRECT
/**
 * @brief Do not wait if file is not available for operation, but fail immediately
*/
#define IO_FILE_FLAG_NO_WAIT IO_VFS_OPERATION_FLAG_NO_WAIT

EXPORT
/**
 * @brief File open mode
 * @note Any mode can be used with IO_FILE_BINARY option
*/
typedef enum
{
    IO_FILE_READ = 0,
    IO_FILE_WRITE = 1,
    IO_FILE_APPEND = 2,
    IO_FILE_READ_ATTRIBUTES = 4,
    IO_FILE_WRITE_ATTRIBUTES = 8,
    IO_FILE_BINARY = 0x8000,
} IoFileOpenMode;

EXPORT
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

EXPORT
/**
 * @brief Open file in kernel mode
 * @param *file File path string
 * @param mode File open mode
 * @param flags File flags
 * @param **handle Output file handle
 * @return Status code
*/
EXTERN STATUS IoOpenKernelFile(char *file, IoFileOpenMode mode, IoFileFlags flags, struct IoFileHandle **handle);

EXPORT
/**
 * @brief Close file in kernel mode
 * @param *handle File handle
 * @return Status code
*/
EXTERN STATUS IoCloseKernelFile(struct IoFileHandle *handle);

EXPORT
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
EXTERN STATUS IoReadFile(struct KeTaskControlBlock *task, int handle, void *buffer, uint64_t size, uint64_t offset, 
    IoReadWriteCompletionCallback callback, void *context);

EXPORT
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
EXTERN STATUS IoReadKernelFile(struct IoFileHandle *handle, void *buffer, uint64_t size, uint64_t offset,
    IoReadWriteCompletionCallback callback, void *context);

EXPORT
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
EXTERN STATUS IoWriteFile(struct KeTaskControlBlock *task, int handle, void *buffer, uint64_t size, uint64_t offset,
    IoReadWriteCompletionCallback callback, void *context);

EXPORT
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
EXTERN STATUS IoWriteKernelFile(struct IoFileHandle *handle, void *buffer, uint64_t size, uint64_t offset, 
    IoReadWriteCompletionCallback callback, void *context);

EXPORT
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
EXTERN STATUS IoReadWriteKernelFileSync(struct IoFileHandle *handle, void *buffer, uint64_t size, uint64_t offset, 
    uint64_t *actualSize, bool write);

EXPORT
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
EXTERN STATUS IoReadWriteFileSync(struct KeTaskControlBlock *task, int handle, void *buffer, uint64_t size, uint64_t offset, 
    uint64_t *actualSize, bool write);

EXPORT
/**
 * @brief Read file in kernel mode (globally) synchronously
 * @param *handle File handle
 * @param *buffer Source buffer
 * @param size Count of bytes to write
 * @param offset Offset into the file in bytes
 * @param *actualSize Count of bytes actually read
 * @return Status code
*/
EXTERN STATUS IoReadKernelFileSync(struct IoFileHandle *handle, void *buffer, uint64_t size, uint64_t offset, uint64_t *actualSize);

EXPORT
/**
 * @brief Write file in kernel mode (globally) synchronously
 * @param *handle File handle
 * @param *buffer Source buffer
 * @param size Count of bytes to write
 * @param offset Offset into the file in bytes
 * @param *actualSize Count of bytes actually written
 * @return Status code
*/
EXTERN STATUS IoWriteKernelFileSync(struct IoFileHandle *handle, void *buffer, uint64_t size, uint64_t offset, uint64_t *actualSize);

EXPORT
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
EXTERN STATUS IoReadFileSync(struct KeTaskControlBlock *task, int handle, void *buffer, uint64_t size, uint64_t offset, uint64_t *actualSize);

EXPORT
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
EXTERN STATUS IoWriteFileSync(struct KeTaskControlBlock *task, int handle, void *buffer, uint64_t size, uint64_t offset, uint64_t *actualSize);

EXPORT
/**
 * @brief Check if file exists
 * @param *file File path
 * @return True if exists, false if not
*/
EXTERN bool IoCheckIfFileExists(char *file);

EXPORT
/**
 * @brief Get file size
 * @param *file File path
 * @param *size Pointer to where to store size
 * @return Status code
*/
EXTERN STATUS IoGetFileSize(char *file, uint64_t *size);

/**
 * @brief Initialize I/O File Manager subsystem
 * @return Status code
*/
INTERNAL STATUS IoFsInit(void);

#endif