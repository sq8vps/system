#ifndef EX_FSDRV_H_
#define EX_FSDRV_H_

#include <stdint.h>
#include "defines.h"

struct IoDeviceObject;

INTERNAL STATUS ExMountVolume(struct IoDeviceObject *disk);

#endif