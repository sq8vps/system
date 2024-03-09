#ifndef PART_H_
#define PART_H_

#include <stdint.h>
#include "defines.h"
#include "disk.h"
#include "io/dev/dev.h"

STATUS DiskInitializeVolume(struct IoDeviceObject *bdo, struct DiskData *info);

#endif