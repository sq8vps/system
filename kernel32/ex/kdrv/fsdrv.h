/**
 * @file fsdrv.h
 * @brief File system driver/device specific routines
 * @ingroup kdrv
 */

#ifndef EX_FSDRV_H_
#define EX_FSDRV_H_

#include <stdint.h>
#include "defines.h"

struct IoVolumeNode;

/**
 * @addtogroup kdrv
 * @ingroup exec
 * @{
 */

/**
 * @brief Load filesystem driver and build device stack for a volume
 * @param *volume Volume pointer
 * @kinternal
 * @return Status code
 */
INTERNAL STATUS ExMountVolume(struct IoVolumeNode *volume);

/**
 * @}
 */

#endif