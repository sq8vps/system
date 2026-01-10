/**
 * @file config.h
 * @brief Kernel configuration - static and command line parsing
 * @ingroup config
 */

#ifndef KERNEL_CONFIG_H_
#define KERNEL_CONFIG_H_

#include "defines.h"
#include "hal/arch.h"

/**
 * @addtogroup config Kernel configuration
 * @brief Kernel configuration
 * 
 * This module defines some default values for the kernel. These values can be, in general, changed freely.
 * This module also parses kernel command line arguments.
 * @{
 */

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
 * @brief Name of the master configuration database
 */
#define CONFIG_DATABASE_NAME "config.ndb"

/**
 * @brief Initial (pre-disk) configuration database path
 */
#define INITIAL_CONFIG_DATABASE (INITRD_MOUNT_POINT "/" CONFIG_DATABASE_NAME)

/**
 * @brief Main filesystem mount point name
 */
#define MAIN_MOUNT_POINT "/main"

/**
 * @brief Main configuration database path
 */
#define CONFIG_DATABASE (MAIN_MOUNT_POINT "/system/config/" CONFIG_DATABASE_NAME)

/**
 * @brief Get kernel command line parameter by name
 * @param *name Parameter name
 * @param **value Output value for parameter @a name (null-safe)
 * @return True if parameter found, false otherwise
 * @note If there is no value for the given parameter, @a *value is set to NULL
 */
bool ConfigGetKernelParam(const char *name, const char **value);

END_EXPORT_API

/**
 * @brief Kernel name
 */
#define KERNEL_NAME_STRING "Nabla"

/**
 * @brief Kernel version
 */
#define KERNEL_VERSION_STRING "0.1"

#ifdef DEBUG
    /**
     * @brief Kernel compilation type name
     */
    #define KERNEL_COMPILATION_TYPE_STRING "debug"
#else
    /**
     * @brief Kernel compilation type name
     */
    #define KERNEL_COMPILATION_TYPE_STRING "release"
#endif

#ifdef SMP
    /**
     * @brief Kernel processor mode name
     */
    #define KERNEL_CPU_TYPE_STRING "multiprocessor"
#else
    /**
     * @brief Kernel processor mode name
     */
    #define KERNEL_CPU_TYPE_STRING "uniprocessor"
#endif

/**
 * @brief Full kernel name string
 */
#define KERNEL_FULL_NAME_STRING (KERNEL_NAME_STRING " " KERNEL_VERSION_STRING " " HAL_ARCHITRECTURE_STRING " " KERNEL_CPU_TYPE_STRING  " " KERNEL_COMPILATION_TYPE_STRING)

/**
 * @brief Get and parse kernel command line arguments
 * @param *bootData Bootloader data
 */
INTERNAL void ConfigParseKernelArguments(void *bootData);

/**
 * @}
 */

#endif