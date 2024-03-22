#ifndef PART_H_
#define PART_H_

#include <stdint.h>
#include "defines.h"
#include "disk.h"
#include "io/dev/dev.h"



STATUS DiskCreateDeviceFile(struct IoDeviceObject *dev, struct DiskData *info);

STATUS DiskInitializeVolume(struct IoDeviceObject *bdo, struct IoDeviceObject *dev, struct DiskData *info);

#endif