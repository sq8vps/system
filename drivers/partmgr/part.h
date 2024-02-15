#ifndef PART_H_
#define PART_H_

#include <stdint.h>
#include <stdbool.h>
#include "mbr.h"
#include "gpt.h"
#include "io/dev/types.h"

enum PartmgrScheme
{
    SCHEME_NONE = 0,
    SCHEME_MBR = 1,
    SCHEME_GPT = 2,
    SCHEME_UNKNOWN = 0xFFFF
};

struct PartmgrDriveData
{
    struct IoSubDeviceObject *device;
    bool usable;
    enum PartmgrScheme scheme;
    union
    {
        struct Mbr *mbr;
    };
    struct IoAccessParameters ioParams;
};

STATUS PartmgrInitialize(struct PartmgrDriveData *info);

#endif