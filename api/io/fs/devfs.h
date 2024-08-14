//This header file is generated automatically
#ifndef EXPORTED___API__IO_FS_DEVFS_H_
#define EXPORTED___API__IO_FS_DEVFS_H_

#ifdef __cplusplus
extern "C" 
{
#endif

#include <stdint.h>
#include "defines.h"
#include "io/fs/vfs.h"

struct IoDeviceObject;

/**
 * @brief Create device file
 * @param *dev Device object
 * @param flags VFS flags
 * @param *name File name, must be unique in \a /dev
 * @return Status code
*/
STATUS IoCreateDeviceFile(struct IoDeviceObject *dev, IoVfsNodeFlags flags, char *name);


#ifdef __cplusplus
}
#endif

#endif