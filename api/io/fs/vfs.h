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
#include "ob/ob.h"
struct ObObjectHeader;

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