/**
 * @file devfs.h
 * @brief Device file system (\c /dev) supprt
 * @ingroup io_fs
 */

#ifndef KERNEL_DEVFS_H_
#define KERNEL_DEVFS_H_

#include <stdint.h>
#include "defines.h"
#include "io/fs/vfs.h"

DRIVER_API

struct IoDeviceObject;

/**
 * @addtogroup io_fs_devfs Device file system (\c /dev) support
 * @brief Device file system (\c /dev) support
 * @ingroup io_fs
 * @{
 */

/**
 * @brief Create device file
 * @param *dev Device object
 * @param flags VFS flags
 * @param *name File name, must be unique in \a /dev
 * @return Status code
*/
STATUS IoCreateDeviceFile(struct IoDeviceObject *dev, enum IoVfsFlags flags, char *name);

END_DRIVER_API

/**
 * @brief Initialize "/dev" filesystem
 * @param *root Root filesystem node
 * @kinternal
 * @return Status code
*/
INTERNAL STATUS IoInitDeviceFs(struct IoVfsNode *root);

/**
 * @}
 */

#endif