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

/**
 * @brief Volume node structure
*/

struct IoVolumeNode
{
    OBJECT;
    char label[IO_VOLUME_MAX_LABEL_LENGTH + 1]; /**< Volume label */
    struct IoDeviceObject *fsdo; /**< FS Device Object */
    struct IoDeviceObject *pdo; /**< Physical Device Object */
    struct IoVfsNode *mountPoint; /**< Filesystem mount point */
    uint64_t serialNumber; /**< Volume serial number */
    enum IoDeviceStatus status;
    enum IoDeviceStatusFlags flags;

    struct IoVolumeNode *next, *previous; /**< A list of other volumes */
};


/**
 * @brief Mount volume
 * @param *devPath Volume device path
 * @param *mountPoint Mount point name
 * @return Status code
*/
STATUS IoMountVolume(const char *devPath, const char *mountPoint);

/**
 * @brief Mount volume
 * @param *dev Disk device object
 * @param *mountPoint Mount point name
 * @return Status code
*/
STATUS IoMountVolumeByDevice(struct IoDeviceObject *dev, const char *mountPoint);

/**
 * @brief Register volume associated with a device
 * 
 * Register new volume associated with given the device. The target device
 * must be a disk device (\a IO_DEVICE_TYPE_DISK) and must not have any volume
 * already associated.
 * @param *dev Associated device
 * @return Status code
*/
STATUS IoRegisterVolume(struct IoDeviceObject *dev);


/**
 * @brief Set volume serial number
 * @param *dev Device object with associated volume
 * @param serial Serial number
 * @return Status code
 * @note This function fails when there is no associated volume
*/
STATUS IoSetVolumeSerialNumber(struct IoDeviceObject *dev, uint64_t serial);


/**
 * @brief Set volume label
 * @param *dev Device object with associated volume
 * @param *label Volume label (length <= \a IO_VOLUME_MAX_LABEL_LENGTH)
 * @return Status code
 * @note This function fails when there is no associated volume
*/
STATUS IoSetVolumeLabel(struct IoDeviceObject *dev, char *label);


/**
 * @brief Register filesystem for given volume
 * @param *disk Volume device object (type = \a IO_DEVICE_TYPE_DISK)
 * @param *fs Filesystem device object (type = \a IO_DEVICE_TYPE_FS)
 * @return Status code
*/
STATUS IoRegisterFilesystem(struct IoDeviceObject *disk, struct IoDeviceObject *fs);



#ifdef __cplusplus
}
#endif

#endif