#ifndef DISK_H_
#define DISK_H_

#include <stdint.h>
#include <stdbool.h>
#include "io/dev/rp.h"

struct DiskData
{
    bool usable;
    struct IoAccessParameters ioParams;
};

STATUS DiskReadWrite(struct IoDriverRp *rp);

#endif