#ifndef KERNEL_KE_CORE_SYS_FILE_H_
#define KERNEL_KE_CORE_SYS_FILE_H_

#include "defines.h"

EXPORT_SYSCALL

#include <stddef.h>
#include <stdint.h>

#define SYSCALL_OPEN 2 /**< Open file */
#define SYSCALL_CLOSE 3 /**< Close file */
#define SYSCALL_READ 4 /**< Read from file */
#define SYSCALL_WRITE 5 /**< Write to file */

/**
 * @brief @ref open() flags
 */
enum
{
    FILE_FLAG_DIRECT = 0x1, /**< Force performing operation directly (omitting internal bufferring), fail if not possible */
    FILE_FLAG_NO_WAIT = 0x2, /**< Do not wait if file is not available for operation, but fail immediately */
    FILE_FLAG_SHARED = 0x4, /**< Allow other processes to use the file regardless of mode and locking policy */
    FILE_NO_LINK_RESOLUTION = 0x10, /**< If the target file is a link, do not resolve it, but rather work on the link itself */
};

/**
 * @brief @ref open() modes
 * 
 * File content is available for reading in \a IO_FILE_READ, \a IO_FILE_WRITE and \a IO_FILE_APPEND modes.
 * \a IO_FILE_WRITE allows to write to any offset without destroying current file content (except the content being explicitly overwritten).
 * \a IO_FILE_APPEND allows to append content to the end and ignores provided offset.
 * \a IO_FILE_REPLACE flag can be ORed with open mode to destroy current file content.
 * By default, when the file does not exist, the function fails and nothing is read nor written.
 * \a IO_FILE_CREATE flag ORed with open mode allows to create the file if it does not exist.
*/
enum
{
    FILE_READ = 0x0, /**< Open file for reading */
    FILE_WRITE = 0x1, /**< Open file for writing (to any offset) */
    FILE_APPEND = 0x2, /**< Open file for appending (offset ignored) */
    FILE_CREATE = 0x10, /**< Create file if does not exist */
    FILE_REPLACE = 0x20, /**< Replace file content on writing (destroy old content) */
};

/**
 * @brief Open file
 * @param *file File path string
 * @param mode File open mode
 * @param flags File flags
 * @return File handle or -1 on failure
*/
int open(const char *file, int mode, int flags);

/**
 * @brief Close file
 * @param handle File handle
 * @return Zero on success or -1 on failure
 */
int close(int handle);

/**
 * @brief Read file
 * @param handle File handle
 * @param *buffer Destination buffer
 * @param size Count of bytes to read (max size)
 * @param offsetLo Offset into the file in bytes (low part)
 * @param offsetHi Offset into the file in bytes (high part - ignored when size_t is >=64-bit)
 * @return Count of bytes read or -1 on failure
 */
size_t read(int handle, void *buffer, size_t size, size_t offsetLo, size_t offsetHi);

/**
 * @brief Write file
 * @param handle File handle
 * @param *buffer Source buffer
 * @param size Count of bytes to write
 * @param offsetLo Offset into the file in bytes (low part, ignored in append mode)
 * @param offsetHi Offset into the file in bytes (high part - ignored when size_t is >=64-bit; ignored in append mode)
 * @return Count of bytes written or -1 on failure
 */
size_t write(int handle, const void *buffer, size_t size, size_t offsetLo, size_t offsetHi);

END_EXPORT_SYSCALL

INTERNAL reg_t KeSyscallOpen(reg_t arg1, reg_t arg2, reg_t arg3, reg_t arg4, reg_t arg5);
INTERNAL reg_t KeSyscallClose(reg_t arg1, reg_t arg2, reg_t arg3, reg_t arg4, reg_t arg5);
INTERNAL reg_t KeSyscallRead(reg_t arg1, reg_t arg2, reg_t arg3, reg_t arg4, reg_t arg5);
INTERNAL reg_t KeSyscallWrite(reg_t arg1, reg_t arg2, reg_t arg3, reg_t arg4, reg_t arg5);

#endif