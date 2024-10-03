#ifndef KERNEL_CONFIG_H_
#define KERNEL_CONFIG_H_

#include "defines.h"

EXPORT_API

/**
 * @brief Max CPU number handled by the kernel
 */
#define MAX_CPU_COUNT 64

/**
 * @brief Max count of kernel mode tasks associated with one process 
 */
#define MAX_KERNEL_MODE_THREADS 48

/**
 * @brief Initial ramdisk mount point
 */
#define INITRD_MOUNT_POINT "/initrd"

#define INITIAL_CONFIG_DATABASE (INITRD_MOUNT_POINT "/config.ndb")

END_EXPORT_API

#endif