//This header file is generated automatically
#ifndef EXPORTED___API__IO_DEV_VOL_H_
#define EXPORTED___API__IO_DEV_VOL_H_

#ifdef __cplusplus
extern "C" 
{
#endif

#include <stdint.h>
#include "io/dev/dev.h"
#include "ob/ob.h"
/**
 * @brief Maximum length of volume label
*/
#define IO_VOLUME_MAX_LABEL_LENGTH 32

struct IoVolumeNode
{
    struct ObObjectHeader objectHeader;
    char label[IO_VOLUME_MAX_LABEL_LENGTH + 1]; /**< Volume label */
    IoDeviceFlags flags; /**< Common volume flags */
    struct IoDeviceObject *fsdo; /**< FS Device Object */
    struct IoDeviceObject *pdo; /**< Physical Device Object */
    struct IoVfsNode *mountPoint; /**< Filesystem mount point */
    uint64_t serialNumber; /**< Volume serial number */

    struct IoVolumeNode *next, *previous; /**< A list of other volumes */
};

/**
 * @brief Register volume associated with a device
 * 
 * Register new volume associated with given the device. The target device
 * must be a disk device (\a IO_DEVICE_TYPE_DISK) and must not have any volume
 * already associated.
 * @param *dev Associated device
 * @param flags Volume node flags
 * @return Status code
 * @attention Priority level must be <= \a HAL_PRIORITY_LEVEL_DPC
*/
extern STATUS IoRegisterVolume(struct IoDeviceObject *dev, IoDeviceFlags flags);


#ifdef __cplusplus
}
#endif

#endif