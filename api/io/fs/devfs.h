//This header file is generated automatically
#ifndef EXPORTED___API__IO_FS_DEVFS_H_
#define EXPORTED___API__IO_FS_DEVFS_H_

#ifdef __cplusplus
extern "C" 
{
#endif

#include <stdint.h>
#include "defines.h"
#include "io/dev/dev.h"
#include "io/fs/vfs.h"
#include "fstypedefs.h"
/**
 * @brief Create device file
 * @param *dev Device object
 * @param ref File reference value for driver
 * @param flags VFS flags
 * @param *name File name, must be unique in \a /dev
 * @return Status code
*/
extern STATUS IoCreateDeviceFile(struct IoDeviceObject *dev, union IoVfsReference ref, IoVfsFlags flags, char *name);


#ifdef __cplusplus
}
#endif

#endif