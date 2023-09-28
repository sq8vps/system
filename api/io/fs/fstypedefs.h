//This header file is generated automatically
#ifndef EXPORTED___API__IO_FS_FSTYPEDEFS_H_
#define EXPORTED___API__IO_FS_FSTYPEDEFS_H_

#ifdef __cplusplus
extern "C" 
{
#endif

#include <stdint.h>
typedef uint32_t IoVfsOperationFlags;

#define IO_VFS_OPERATION_FLAG_SYNCHRONOUS 1 //synchronous read/write, that is do no return until completed
#define IO_VFS_OPERATION_FLAG_DIRECT 2 //unbuffered read/write
#define IO_VFS_OPERATION_FLAG_NO_WAIT 4 //do not wait if file is locked


#ifdef __cplusplus
}
#endif

#endif