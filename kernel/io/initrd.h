/**
 * @file initrd.h
 * @brief Read-only initial ramdisk driver
 * @ingroup io
 */

#ifndef KERNEL_INITRD_H_
#define KERNEL_INITRD_H_

#include "defines.h"
#include <stdint.h>

struct IoVfsNode;

/**
 * @addtogroup io_initrd Read-only initial ramdisk driver
 * @brief Read-only initial ramdisk driver
 * @ingroup io
 * @kinternal
 * @{
 */

/**
 * @brief Initialize intial ramdisk
 * @param *bootArgs Bootloader data
 * @return Status code
 */
INTERNAL STATUS IoInitrdInit(void *bootArgs);

/**
 * @brief Mount initial ramdisk
 * @param *mountPoint Mount point path
 * @return Status code
 */
INTERNAL STATUS IoInitrdMount(char *mountPoint);

/**
 * @brief Get initial ramdisk file node
 * @param *parent Parent VFS node
 * @param *name File name
 * @param **node Output node, allocated by the function
 * @return Status code
 */
INTERNAL STATUS IoInitrdGetNode(struct IoVfsNode *parent, const char *name, struct IoVfsNode **node);

/**
 * @brief Read file from the initial ramdisk
 * @param *node File VFS node
 * @param *buffer Destination buffer
 * @param size Count of bytes to read
 * @param offset Offset in bytes
 * @return Count of bytes actually read
 */
INTERNAL size_t IoInitrdRead(const struct IoVfsNode *node, void *buffer, size_t size, size_t offset);

/**
 * @}
 */

#endif