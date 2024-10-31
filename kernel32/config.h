#ifndef KERNEL_CONFIG_H_
#define KERNEL_CONFIG_H_

#include "defines.h"
#include "hal/arch.h"

EXPORT_API

/**
 * @brief Max CPU number handled by the kernel
 */
#define MAX_CPU_COUNT 64

/**
 * @brief Initial ramdisk mount point
 */
#define INITRD_MOUNT_POINT "/initrd"

/**
 * @brief Initial configuration database path
 */
#define INITIAL_CONFIG_DATABASE (INITRD_MOUNT_POINT "/config.ndb")

END_EXPORT_API

#define KERNEL_NAME_STRING "Nabla"
#define KERNEL_VERSION_STRING "0.1"
#ifdef DEBUG
    #define KERNEL_COMPILATION_TYPE_STRING "debug"
#else
    #define KERNEL_COMPILATION_TYPE_STRING "release"
#endif

#ifdef SMP
    #define KERNEL_CPU_TYPE_STRING "multiprocessor"
#else
    #define KERNEL_CPU_TYPE_STRING "uniprocessor"
#endif

#define KERNEL_FULL_NAME_STRING (KERNEL_NAME_STRING " " KERNEL_VERSION_STRING " " HAL_ARCHITRECTURE_STRING " " KERNEL_CPU_TYPE_STRING  " " KERNEL_COMPILATION_TYPE_STRING)

#endif