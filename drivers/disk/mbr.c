#include "mbr.h"
#include "assert.h"
#include "common/order.h"
#include "disk.h"
#include "rtl/uuid.h"
#include "common/order.h"

#define MBR_COPY_PROTECTION_FLAG 0x5A5A
#define MBR_BOOT_SIGNATURE 0xAA55
#define MBR_BOOT_FLAG 0x80

#define MBR_UUID_HEADER {0xb9, 0xa4, 0x2e, 0x5b, 0xfc, 0x03, 0x4b, 0x8c, 0x90, 0x37, 0x00, 0x00}

struct RawChs
{
    uint8_t head;
    uint8_t sectorCylinderHi;
    uint8_t cylinderLo;
} PACKED;

struct RawMbr
{
    uint8_t unused[440];
    uint32_t signature;
    uint16_t copyProtectionFlag;
    struct
    {
        uint8_t bootable;
        struct RawChs firstChs;
        uint8_t type;
        struct RawChs lastChs;
        uint32_t lba;
        uint32_t sectors;
    } partition[4] PACKED;
    uint16_t bootSignature;
} PACKED;

bool DiskMbrParse(const void *data, struct Mbr *mbr)
{
    ASSERT(data && mbr);
    const struct RawMbr *rmbr = data;
    if(CmLeU16(rmbr->bootSignature) != MBR_BOOT_SIGNATURE)
        return false;
    mbr->signature = CmLeU32(rmbr->signature);
    mbr->copyProtection = (rmbr->copyProtectionFlag == MBR_COPY_PROTECTION_FLAG);
    for(uint8_t i = 0; i < 4; i++)
    {
        if(MBR_PARTITION_TYPE_EMPTY == rmbr->partition[i].type)
        {
            mbr->partition[i].used = 0;
        }
        else
        {
            mbr->partition[i].used = 1;
            mbr->partition[i].bootable = !!(rmbr->partition[i].bootable & MBR_BOOT_FLAG);
            mbr->partition[i].type = rmbr->partition[i].type;
            mbr->partition[i].lba = CmLeU32(rmbr->partition[i].lba);
            mbr->partition[i].sectors = CmLeU32(rmbr->partition[i].sectors);
            mbr->partition[i].firstChs.head = rmbr->partition[i].firstChs.head;
            mbr->partition[i].firstChs.cylinder = rmbr->partition[i].firstChs.cylinderLo;
            mbr->partition[i].firstChs.cylinder |= ((uint16_t)(rmbr->partition[i].firstChs.sectorCylinderHi & 0xC0) << 2);
            mbr->partition[i].firstChs.sector = rmbr->partition[i].firstChs.sectorCylinderHi & 0x3F;
            mbr->partition[i].lastChs.head = rmbr->partition[i].lastChs.head;
            mbr->partition[i].lastChs.cylinder = rmbr->partition[i].lastChs.cylinderLo;
            mbr->partition[i].lastChs.cylinder |= ((uint16_t)(rmbr->partition[i].lastChs.sectorCylinderHi & 0xC0) << 2);
            mbr->partition[i].lastChs.sector = rmbr->partition[i].lastChs.sectorCylinderHi & 0x3F;
        }
    }
    return true;
}

bool DiskMbrIsPartitionUsable(struct MbrPartition *part)
{
    if(!part->used)
        return false;
    switch(part->type)
    {
        case MBR_PARTITION_TYPE_EMPTY:
        case MBR_PARTITION_TYPE_PROTECTIVE:
            return false;
            break;
        default:
            return true;
            break;
    }
}

STATUS DiskMbrGetUuid(struct DiskData *info, char *str)
{
    if(!info->isMbr)
        return SYSTEM_INCOMPATIBLE;
    
    uint8_t uuid[16] = MBR_UUID_HEADER;
    *(uint32_t*)(uuid + 12) = CmBeU32(info->mbr->signature);

    RtlUuidToString(uuid, str, false);
    return OK;
}