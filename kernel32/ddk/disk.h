/**
 * @file disk.h
 * @brief Disk DDK and helpers
 * @ingroup ddk
 */

#ifndef DDK_DISK_H_
#define DDK_DISK_H_

#include <stdint.h>
#include "defines.h"

EXPORT_API

struct IoDeviceObject;

/**
 * @addtogroup ddk Driver Development Kit (DDK)
 * @brief Kernel-space Driver Development Kit (DDK) routines and definitions
 * 
 * This module constitutes a Driver Development Kit, which defines requestes and associated data structures that must be supported
 * by any compatible driver. This module is partitioned into several files/groups corresponding to different device types.
 * Moreover, this module provides universal helpers/wrappers for these requests, making interfacing between drivers simpler.
 */

/**
 * @addtogroup ddk_disk Disk device requests and helpers
 * @ingroup ddk
 * @note This module applies to abstract disk devices, natively handled, e.g., by the \a disk.drv driver. 
 * The module for storage controllers is stor.h.
 * @{
 */

/**
 * @brief Type specific operations for disk devices
*/
enum DiskOperations
{
    DISK_NONE = 0, /**< No operation */
    DISK_GET_SIGNATURE = 1, /**< Get disk device signature */
};


/**
 * @brief Get disk device signature
 * @param *target Target disk device BDO
 * @param **signature Target disk signature string, allocated by the driver
 * @return Status code
 * @attention This function is always synchronous
*/
STATUS DiskGetSignature(struct IoDeviceObject *target, char **signature);

/**
 * @}
 */

END_EXPORT_API

#endif