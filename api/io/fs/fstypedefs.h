//This header file is generated automatically
#ifndef EXPORTED___API__IO_FS_FSTYPEDEFS_H_
#define EXPORTED___API__IO_FS_FSTYPEDEFS_H_

#ifdef __cplusplus
extern "C" 
{
#endif

#include <stdint.h>
typedef uint32_t IoVfsFlags;
#define IO_VFS_FLAG_READ_ONLY 0x1 //file/directory is read only
#define IO_VFS_FLAG_LOCK 0x2 //node is locked, for example during write - VFS does not check this flag
#define IO_VFS_FLAG_NO_CACHE 0x4 //do not cache this entry
#define IO_VFS_FLAG_VIRTUAL 0x8
#define IO_VFS_FLAG_DIRTY 0x10 //data for this entry changed in cache, must be send to disk first
#define IO_VFS_FLAG_VFS_DIRECTORY 0x10000000
#define IO_VFS_FLAG_MOUNT_POINT 0x20000000
#define IO_VFS_FLAG_PERSISTENT 0x80000000 //persisent entry (unremovable)

typedef uint32_t IoVfsOperationFlags;

#define IO_VFS_OPERATION_FLAG_SYNCHRONOUS 1 //synchronous read/write, that is do no return until completed
#define IO_VFS_OPERATION_FLAG_DIRECT 2 //unbuffered read/write
#define IO_VFS_OPERATION_FLAG_NO_WAIT 4 //do not wait if file is locked


#ifdef __cplusplus
}
#endif

#endif