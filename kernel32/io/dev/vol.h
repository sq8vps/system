#ifndef IO_DEV_VOL_H_
#define IO_DEV_VOL_H_

#include <stdint.h>
#include "io/dev/dev.h"
#include "ob/ob.h"

EXPORT
/**
 * @brief Maximum length of volume label
*/
#define IO_VOLUME_MAX_LABEL_LENGTH 32

/**
 * @brief Volume node structure
*/
EXPORT
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

EXPORT
/**
 * @brief Register volume associated with a device
 * 
 * Register new volume associated with given the device. The target device
 * must be a disk device (\a IO_DEVICE_TYPE_DISK) and must not have any volume
 * already associated.
 * @param *dev Associated device
 * @param flags Volume node flags
 * @return Status code
*/
EXTERN STATUS IoRegisterVolume(struct IoDeviceObject *dev, IoDeviceFlags flags);

EXPORT
/**
 * @brief Set volume serial number
 * @param *dev Device object with associated volume
 * @param serial Serial number
 * @return Status code
 * @note This function fails when there is no associated volume
*/
EXTERN STATUS IoSetVolumeSerialNumber(struct IoDeviceObject *dev, uint64_t serial);

EXPORT
/**
 * @brief Set volume label
 * @param *dev Device object with associated volume
 * @param *label Volume label (length <= \a IO_VOLUME_MAX_LABEL_LENGTH)
 * @return Status code
 * @note This function fails when there is no associated volume
*/
EXTERN STATUS IoSetVolumeLabel(struct IoDeviceObject *dev, char *label);

#endif