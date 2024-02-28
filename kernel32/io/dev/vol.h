#ifndef IO_DEV_VOL_H_
#define IO_DEV_VOL_H_

#include <stdint.h>
#include "dev.h"
#include "typedefs.h"

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
    char label[IO_VOLUME_MAX_LABEL_LENGTH + 1]; /**< Volume label */
    IoDeviceFlags flags; /**< Common volume flags */
    struct IoDeviceObject *fsdo; /**< FS Device Object */
    struct IoDeviceObject *pdo; /**< Physical Device Object */
    struct IoVfsNode *mountPoint; /**< Filesystem mount point */
    uint64_t serialNumber; /**< Volume serial number */

    struct IoVolumeNode *next, *previous; /**< A list of other volumes */
};

#endif