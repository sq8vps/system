#ifndef FAT_STRUCTS_H_
#define FAT_STRUCTS_H_

#include <stdint.h>
#include "defines.h"

#define FAT_BPB_SIGNATURE_VALUE 0xAA55

struct FatBpbExtended16
{
    uint8_t biosDriveNumber;
    uint8_t rsvd1;
    uint8_t bootSignature;
    uint32_t serial;
    char label[11];
    char fsString[8];
    uint8_t rsvd2[448];
} PACKED;

struct FatBpbExtended32
{
    uint32_t fatSize32;
    uint16_t flags;
    uint16_t version;
    uint32_t rootCluster;
    uint16_t fsInfo;
    uint16_t bootSectorCopy;
    uint8_t rsvd1[12];
    uint8_t biosDriveNumber;
    uint8_t rsvd2;
    uint8_t bootSignature;
    uint32_t serial;
    char label[11];
    char fsString[8];
    uint8_t rsvd3[420];
} PACKED;

struct FatBpb
{
    uint8_t jmp[3];
    char oem[8];
    uint16_t bytesPerSector;
    uint8_t sectorsPerCluster;
    uint16_t reservedSectors;
    uint8_t fatCount;
    uint16_t rootEntryCount;
    uint16_t sectors16;
    uint8_t media;
    uint16_t fatSize16;
    uint16_t sectorsPerTrack;
    uint16_t headCount;
    uint32_t hiddenSectors;
    uint32_t sectors32;
    union
    {
        struct FatBpbExtended16 ext16;
        struct FatBpbExtended32 ext32;
    };
    uint16_t signature;
} PACKED;

#define FAT_FS_INFO_LEAD_SIGNATURE 0x41615252
#define FAT_FS_INFO_STRUC_SIGNATURE 0x61417272
#define FAT_FS_INFO_TRAIL_SIGNATURE 0xAA550000

struct FatFsInfo
{
    uint32_t leadSignature;
    uint8_t rsvd1[480];
    uint32_t strucSignature;
    uint32_t freeCount;
    uint32_t nextFree;
    uint8_t rsvd2[12];
    uint32_t trailSignature;
} PACKED;

enum FatType
{
    FAT12 = 1,
    FAT16 = 2,
    FAT32 = 3,
};

struct IoDeviceObject;

struct FatVolume
{
    struct IoDeviceObject *disk;
    enum FatType type;
    uint8_t sectorsPerCluster;
    uint16_t reservedSectors;
    uint8_t fatCount;
    uint32_t fatSize;
    uint32_t clusters;

    char label[11];
    uint32_t serial;

    struct FatFsInfo fsInfo;

    //FAT12/16 specific
    uint16_t rootEntryCount;

    //FAT32 specific
    uint32_t rootCluster;
    uint16_t fsInfoSector;
};

#endif