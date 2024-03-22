#ifndef MOUNT_H_
#define MOUNT_H_

#include "defines.h"
#include <stdint.h>
#include "ex/kdrv.h"
#include "io/dev/dev.h"

STATUS FatMount(struct ExDriverObject *drv, struct IoDeviceObject *disk);

#endif