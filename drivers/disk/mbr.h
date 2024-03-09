#ifndef MBR_H_
#define MBR_H_

#include <stdint.h>
#include <stdbool.h>
#include "ddk/stor.h"
#include "defines.h"

#define MBR_SIZE_ON_DISK 512
#define MBR_LBA_OFFSET 0

#define MBR_PARTITION_TYPE_EMPTY 0x00
#define MBR_PARTITION_TYPE_PROTECTIVE 0xEE

struct Mbr
{
    uint32_t signature;
    struct
    {
        uint8_t used : 1;
        uint8_t bootable : 1;
        struct StorChs firstChs;
        uint8_t type;
        struct StorChs lastChs;
        uint32_t lba;
        uint32_t sectors;
    } partition[4];
    uint8_t copyProtection : 1;
    uint8_t bootable : 1;
};


STATUS DiskMbrParse(const void *data, struct Mbr *mbr);

#endif