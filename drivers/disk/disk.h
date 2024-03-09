#ifndef DISK_H_
#define DISK_H_

#include <stdint.h>
#include <stdbool.h>
#include "io/dev/rp.h"
#include "io/dev/dev.h"
#include "mbr.h"
#include "ddk/stor.h"
#include "gpt.h"

//this driver handles partitions in general
//it can be an MDO for the partition 0 and then the BDO is the storage driver
//it can be the BDO and MDO simultaneously for other partitions
struct DiskData
{
    uint8_t isMdo : 1; //this object is a MDO, so it represents a partition
    uint8_t isPartition0 : 1; //this object is for partition 0 (flat disk)
    uint8_t isGpt : 1; //is this disk partitioned using GPT scheme
    uint8_t isMbr : 1; //is this disk partitioned using MBR scheme
    struct IoDeviceData *part0device;
    struct Mbr *mbr;
    struct Gpt *gpt;
    struct
    {
        uint64_t start;
        uint64_t size;
        uint64_t sectorSize;
    } partition;
};

STATUS DiskReadWrite(struct IoRp *rp);

#endif