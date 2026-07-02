/**
 * @file fs.h
 * @brief File support
 * @ingroup io_fs
 */

#ifndef KERNEL_FS_H_
#define KERNEL_FS_H_

#include "defines.h"
#include "ob/ob.h"
#include "io/dev/op.h"
#include "taskfs.h"

/**
 * @addtogroup io_fs File system layer
 * @ingroup io
 * 
 * This module is a file system layer. It provides a Virtual File System (VFS) layer to abstract the actual file systems.
 * Also, it handles special file systems such as \a \dev or \a \task. This module also provides high-level 
 * file abstraction - handles and operations. 
 */

/**
 * @addtogroup io_fs_fs High-level file support
 * @brief High-level file support - handles and operations
 * @ingroup io_fs
 * @{
 */

DRIVER_API

struct KeTaskControlBlock;
struct IoVfsNode;

NABLA_API

/**
 * @brief Use current file offset instead of setting it up
 */
#define IO_OFFSET_CURRENT (uint64_t)(-1)

/**
 * @brief File offset setting methods
 * @warning Offsets exceeding the size of the file are not supported properly.
 */
typedef enum
{
    IO_SET_OFFSET = 0, /**< Set offset directly, i.e., from the beginning of the file */
    IO_SET_OFFSET_CUR = 1, /**< Update current offset */
    IO_SET_OFFSET_END = 2, /**< Set offset with regard to the end of the file */
} IoSetFileOffsetMethod;

/**
 * @brief File handle/object flags
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
 * \a IO_FILE_APPEND allows to append content to the end of the file.
 * \a IO_FILE_REPLACE flag can be ORed with open mode to destroy current file content.
 * By default, when the file does not exist, the function fails and nothing is read nor written.
 * \a IO_FILE_CREATE flag ORed with open mode allows to create the file if it does not exist.
 * Using \a IO_FILE_READ_ATTRIBUTES or \a IO_FILE_WRITE_ATTRIBUTES with \a IO_FILE_WRITE 
 * or \a IO_FILE_APPEND is illegal and result with open error.
*/
typedef enum
{
    IO_FILE_READ = 0x0, /**< Open file for reading */
    IO_FILE_WRITE = 0x1, /**< Open file for writing */
    IO_FILE_APPEND = 0x2, /**< Open file for appending (set initial offset to the end of the file) */
    IO_FILE_CREATE = 0x4, /**< Create file if does not exist */
    IO_FILE_REPLACE = 0x8, /**< Replace file content on writing (destroy old content) */
} IoFileOpenMode;

END_NABLA_API

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
    uint64_t offset; /**< Current offset */
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
 * @param offset \ref IO_OFFSET_CURRENT to track offset or explicit value to change offset
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
 * @param offset \ref IO_OFFSET_CURRENT to track offset or explicit value to change offset
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
 * @param offset \ref IO_OFFSET_CURRENT to track offset or explicit value to change offset
 * @param *actualSize Count of bytes actually read (set to nullptr if not needed)
 * @return Status code
*/
STATUS IoReadFileSync(int handle, void *buffer, size_t size, uint64_t offset, size_t *actualSize);


/**
 * @brief Write file synchronously
 * @param handle File handle
 * @param *buffer Source buffer
 * @param size Count of bytes to write
 * @param offset \ref IO_OFFSET_CURRENT to track offset or explicit value to change offset
 * @param *actualSize Count of bytes actually written (set to nullptr if not needed)
 * @return Status code
*/
STATUS IoWriteFileSync(int handle, void *buffer, size_t size, uint64_t offset, size_t *actualSize);

NABLA_API

/**
 * @brief Open file
 * @param *file File path string
 * @param mode File open mode
 * @param flags File flags
 * @param *handleNumber Output file handle or -1 on failure
 * @return Status code
*/
STATUS ApiOpenFile(const char *file, IoFileOpenMode mode, IoFileFlags flags, int *handleNumber);

/**
 * @brief Close file
 * @param handleNumber File handle
 * @return Status code
*/
STATUS ApiCloseFile(int handleNumber);

/**
 * @brief Read file synchronously
 * @param handle File handle
 * @param *buffer Source buffer
 * @param size Count of bytes to write
 * @param offset \ref IO_OFFSET_CURRENT to track offset or explicit value to change offset
 * @param *actualSize Count of bytes actually read
 * @return Status code
*/
STATUS ApiReadFileSync(int handle, void *buffer, size_t size, uint64_t offset, size_t *actualSize);


/**
 * @brief Write file synchronously
 * @param handle File handle
 * @param *buffer Source buffer
 * @param size Count of bytes to write
 * @param offset \ref IO_OFFSET_CURRENT to track offset or explicit value to change offset
 * @param *actualSize Count of bytes actually written
 * @return Status code
*/
STATUS ApiWriteFileSync(int handle, void *buffer, size_t size, uint64_t offset, size_t *actualSize);

/**
 * @brief Create a symbolic link
 * @param *from Path where the link should be placed
 * @param *to Path to where the link should point to
 * @return Status code
 */
STATUS ApiSymlink(const char *from, const char *to);

/**
 * @brief Get/set file offset
 * @param handle File handle
 * @param offset Absolute/relative offset
 * @param method Method of \a offset interpretation: \ref IO_SET_OFFSET, \ref IO_SET_OFFSET_CUR, or \ref IO_SET_OFFSET_END
 * @param *current Resultant absolute file offset. Might be set to \a nullptr if not needed.
 * @return Status code
 */
STATUS ApiSetFileOffset(int handle, int64_t offset, IoSetFileOffsetMethod method, uint64_t *current);

END_NABLA_API

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

END_DRIVER_API

/**
 * @brief Close all files on process exit
 * @param *pcb Target Process Control Block
 * @return Status code
 * @kinternal
 * @attention This function is called only on process termination
 */
INTERNAL STATUS IoCloseAllFilesOnExit(struct KeProcessControlBlock *pcb);

/**
 * @brief Open file for given process
 * @param *pcb Process Control Block
 * @param *file File path string, used if \a fileNode is NULL
 * @param *fileNode VFS node, used instead of \a file if not NULL
 * @param *taskfs Task file system context
 * @param mode File open mode
 * @param flags File flags
 * @param *handleNumber Output file handle or -1 on failure
 * @kinternal
 * @return Status code
*/
INTERNAL STATUS IoOpenFileForProcess(struct KeProcessControlBlock *pcb, const char *file, struct IoVfsNode *fileNode, struct IoTaskFsContext *taskfs, IoFileOpenMode mode, IoFileFlags flags, int *handleNumber);

/**
 * @brief Close file for given process
 * @param *pcb Process Control Block
 * @param handleNumber File handle
 * @kinternal
 * @return Status code
*/
INTERNAL STATUS IoCloseFileForProcess(struct KeProcessControlBlock *pcb, int handleNumber);

/**
 * @brief Clone file handles to the new process from the calling process
 * @param *pcb Target Process Control Block
 * @param targetHandle Handle number in target process
 * @param sourceHandle Handle number in calling process
 * @kinternal
 * @return Status code
 */
INTERNAL STATUS IoCloneFileToNewProcess(struct KeProcessControlBlock *pcb, int targetHandle, int sourceHandle);

/**
 * @brief Get VFS node associated with given file handle from given process
 * @param *pcb Owner Process Control Block
 * @param handle File handle number
 * @kinternal
 * @return Associated VFS node or NULL on failure
 */
INTERNAL struct IoVfsNode* IoGetVfsNodeForFile(struct KeProcessControlBlock *pcb, int handle);

/**
 * @brief Initialize I/O File Manager subsystem
 * @kinternal
 * @return Status code
*/
INTERNAL STATUS IoFsInit(void);

/**
 * @}
 */

#endif