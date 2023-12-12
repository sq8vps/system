//This header file is generated automatically
#ifndef EXPORTED___API__IO_DEV_TYPEDEFS_H_
#define EXPORTED___API__IO_DEV_TYPEDEFS_H_

#ifdef __cplusplus
extern "C" 
{
#endif

#include <stdint.h>
typedef uint32_t IoDeviceFlags;
#define IO_DEVICE_FLAG_INITIALIZED 0x1
#define IO_DEVICE_FLAG_READY_TO_RUN 0x2
#define IO_DEVICE_FLAG_INITIALIZATION_FAILURE 0x100
#define IO_DEVICE_FLAG_PERSISTENT 0x80000000
#define IO_DEVICE_FLAG_REMOVABLE_MEDIA 0x80000001
#define IO_SUBDEVICE_FLAG_PERSISTENT (IO_DEVICE_FLAG_PERSISTENT)

typedef uint32_t IoDriverRpFlags;
#define IO_DRIVER_RP_FLAG_SYNCHRONOUS 0x01


#ifdef __cplusplus
}
#endif

#endif