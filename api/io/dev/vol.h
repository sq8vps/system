//This header file is generated automatically
#ifndef EXPORTED___API__IO_DEV_VOL_H_
#define EXPORTED___API__IO_DEV_VOL_H_

#ifdef __cplusplus
extern "C" 
{
#endif

#include <stdint.h>
#include "dev.h"
#include "types.h"
/**
 * @brief Maximum length of volume label
*/
#define IO_VOLUME_MAX_LABEL_LENGTH 32

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


#ifdef __cplusplus
}
#endif

#endif