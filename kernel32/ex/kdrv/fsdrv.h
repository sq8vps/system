#ifndef EX_FSDRV_H_
#define EX_FSDRV_H_

#include <stdint.h>
#include "defines.h"

struct IoVolumeNode;

/**
 * @brief Load filesystem driver and build device stack for a volume
 * @param *volume Volume pointer
 * @return Status code
 */
INTERNAL STATUS ExMountVolume(struct IoVolumeNode *volume);

#endif