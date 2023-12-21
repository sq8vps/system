#ifndef KERNEL_MOUNT_H_
#define KERNEL_MOUNT_H_

#include <stdint.h>
#include <stdbool.h>
#include "defines.h"

/**
 * @brief Mount volume at given path
 * @param *device Path to the device being mounted
 * @param *mountPointName Destination mount point name (under \a /), A-Z,a-z,0-9,_,-
 * @return Status code
*/
STATUS IoMountVolume(char *device, char *mountPointName);

/**
 * @brief Unmount volume
 * @param *mountPoint Volume mount point
 * @return Status code
*/
STATUS IoUnmountVolume(char *mountPoint);

#endif