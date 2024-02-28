#ifndef DISK_H_
#define DISK_H_

#include <stdint.h>
#include <stdbool.h>
#include "io/dev/rp.h"
#include "io/dev/dev.h"

struct DiskData
{
    bool usable;
    struct IoDeviceObject *bdo;
};

STATUS DiskReadWrite(struct IoRp *rp);

#endif