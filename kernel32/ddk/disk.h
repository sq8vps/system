#ifndef DDK_DISK_H_
#define DDK_DISK_H_

#include <stdint.h>
#include "defines.h"

EXPORT_API

struct IoDeviceObject;


/**
 * @brief Type specific operations for disk devices
*/
enum DiskOperations
{
    DISK_NONE = 0,
    DISK_GET_SIGNATURE,
};


/**
 * @brief Get disk device signature
 * @param *target Target disk device BDO
 * @param **signature Target disk signature string, allocated by the driver
 * @return Status code
 * @attention This function is always synchronous
*/
STATUS DiskGetSignature(struct IoDeviceObject *target, char **signature);

END_EXPORT_API

#endif