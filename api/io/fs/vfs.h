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
typedef uint32_t IoVfsFlags;
#define IO_VFS_FLAG_READ_ONLY 0x1 //file/directory is read only
#define IO_VFS_FLAG_LOCK 0x2 //node is locked, for example during write - VFS does not check this flag
#define IO_VFS_FLAG_NO_CACHE 0x4 //do not cache this entry
#define IO_VFS_FLAG_VIRTUAL 0x8
#define IO_VFS_FLAG_DIRTY 0x10 //data for this entry changed in cache, must be send to disk first
#define IO_VFS_FLAG_VFS_DIRECTORY 0x10000000
#define IO_VFS_FLAG_PERSISTENT 0x80000000 //persisent entry (unremovable)

typedef uint32_t IoVfsOperationFlags;

#define IO_VFS_OPERATION_FLAG_SYNCHRONOUS 1 //synchronous read/write, that is do no return until completed
#define IO_VFS_OPERATION_FLAG_DIRECT 2 //force unbuffered read/write
#define IO_VFS_OPERATION_FLAG_NO_WAIT 4 //do not wait if file is locked
#define IO_VFS_OPERATION_FLAG_CREATE 8 //create file if doesn't exist

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
 * @brief Get maximum file name length
 * @return Maximum file name length, excluding terminator
*/
extern uint32_t IoVfsGetMaxFileNameLength(void);


#ifdef __cplusplus
}
#endif

#endif