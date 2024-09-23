#ifndef KERNEL_INITRD_H_
#define KERNEL_INITRD_H_

#include "defines.h"
#include <stdint.h>

struct Multiboot2InfoHeader;
struct IoVfsNode;

/**
 * @brief Initialize intial ramdisk
 * @param *mb2h Multiboot2 info table
 * @return Status code
 */
INTERNAL STATUS IoInitrdInit(const struct Multiboot2InfoHeader *mb2h);

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
INTERNAL uintptr_t IoInitrdRead(const struct IoVfsNode *node, void *buffer, uintptr_t size, uintptr_t offset);

#endif