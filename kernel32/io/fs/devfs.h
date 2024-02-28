#ifndef KERNEL_DEVFS_H_
#define KERNEL_DEVFS_H_

#include <stdint.h>
#include "defines.h"
#include "io/dev/dev.h"
#include "io/fs/vfs.h"
#include "fstypedefs.h"

/**
 * @brief Initialize "/dev" filesystem
 * @param *root Root filesystem node
 * @return Status code
*/
INTERNAL STATUS IoInitDeviceFs(struct IoVfsNode *root);

EXPORT
/**
 * @brief Create device file
 * @param *dev Device object
 * @param ref File reference value for driver
 * @param flags VFS flags
 * @param *name File name, must be unique in \a /dev
 * @return Status code
*/
EXTERN STATUS IoCreateDeviceFile(struct IoDeviceObject *dev, union IoVfsReference ref, IoVfsFlags flags, char *name);

#endif