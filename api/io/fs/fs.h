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
#include "fstypedefs.h"
struct KeTaskControlBlock;

/**
 * @brief File operation flags
*/
#define IoFileFlags IoVfsOperationFlags

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

/**
 * @brief File handle
*/
typedef struct IoFileHandle
{
    bool free; /**< Does this entry represent free handle range? */
    union
    {
        struct
        {
            int id; /**< File descriptor */
            struct IoVfsNode *node; /**< VFS node that this file references to */
            uint64_t offset; /**< Current offset in the file (seek())*/
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
 * @brief Open file in kernel mode
 * @param *file File path string
 * @param mode File open mode
 * @param flags File flags
 * @param **handle Output file handle
 * @return Status code
*/
extern STATUS IoOpenKernelFile(char *file, IoFileOpenMode mode, IoFileFlags flags, struct IoFileHandle **handle);

/**
 * @brief Close file in kernel mode
 * @param *handle File handle
 * @return Status code
*/
extern STATUS IoCloseKernelFile(struct IoFileHandle *handle);

/**
 * @brief Read file
 * @param *task Task Control Block
 * @param handle File handle
 * @param *buffer Destination buffer
 * @param size Count of bytes to read (max size)
 * @param offset Offset into the file in bytes
 * @param *actualSize Count of bytes actually read
 * @return Status code
*/
extern STATUS IoReadFile(struct KeTaskControlBlock *task, int handle, void *buffer, uint64_t size, uint64_t offset, uint64_t *actualSize);

/**
 * @brief Read file in kernel mode (globally)
 * @param *handle File handle
 * @param *buffer Destination buffer
 * @param size Count of bytes to read (max size)
 * @param offset Offset into the file in bytes
 * @param *actualSize Count of bytes actually read
 * @return Status code
*/
extern STATUS IoReadKernelFile(struct IoFileHandle *handle, void *buffer, uint64_t size, uint64_t offset, uint64_t *actualSize);

/**
 * @brief Write file
 * @param *task Task Control Block
 * @param handle File handle
 * @param *buffer Source buffer
 * @param size Count of bytes to write
 * @param offset Offset into the file in bytes
 * @param *actualSize Count of bytes actually written
 * @return Status code
*/
extern STATUS IoWriteFile(struct KeTaskControlBlock *task, int handle, void *buffer, uint64_t size, uint64_t offset, uint64_t *actualSize);

/**
 * @brief Write file in kernel mode (globally)
 * @param *handle File handle
 * @param *buffer Source buffer
 * @param size Count of bytes to write
 * @param offset Offset into the file in bytes
 * @param *actualSize Count of bytes actually written
 * @return Status code
*/
extern STATUS IoWriteKernelFile(struct IoFileHandle *handle, void *buffer, uint64_t size, uint64_t offset, uint64_t *actualSize);

/**
 * @brief Check if file exists
 * @param *file File path
 * @return True if exists, false if not
*/
extern bool IoCheckIfFileExists(char *file);

/**
 * @brief Get file size
 * @param *file File path
 * @param *size Pointer to where to store size
 * @return Status code
*/
extern STATUS IoGetFileSize(char *file, uint64_t *size);


#ifdef __cplusplus
}
#endif

#endif