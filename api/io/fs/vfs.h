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
#include "ke/core/mutex.h"
#include "fstypedefs.h"
union IoVfsReference
{
    void *pv;
    char *pc;
    uint64_t u64;
    int64_t i64;
    uint32_t u32;
    int32_t i32;
    uint8_t u8;
    int8_t i8;
};


#ifdef __cplusplus
}
#endif

#endif