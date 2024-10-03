#ifndef FAT_STRUCTS_H_
#define FAT_STRUCTS_H_

#include <stdint.h>
#include "defines.h"
#include "ke/core/mutex.h"

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
    struct IoDeviceObject *vol;
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

    void *fat;
    KeSpinlock fatLock;
    uint32_t modifiedClusterLow;
    uint32_t modifiedClusterHigh;
};

#define FAT_GET_OFFSET(vol, cluster) ((((cluster) - 2) * (vol)->sectorsPerCluster + (vol)->fatCount * (vol)->fatSize + (vol)->reservedSectors) * (vol)->disk->blockSize)

struct FatDirectory
{
    char name[11];
    uint8_t attributes;
    uint8_t rsvd1;
    uint8_t crtTimeTenth;
    uint16_t crtTime;
    uint16_t crtDate;
    uint16_t lastAccDate;
    uint16_t fstClusHi;
    uint16_t wrtTime;
    uint16_t wrtDate;
    uint16_t fstClusLo;
    uint32_t fileSize;
} PACKED;

#define FAT_ATTR_READ_ONLY 0x01
#define FAT_ATTR_HIDDEN 0x02
#define FAT_ATTR_SYSTEM 0x04
#define FAT_ATTR_VOLUME_ID 0x08
#define FAT_ATTR_DIRECTORY 0x10
#define FAT_ATTR_ARCHIVE 0x20
#define FAT_ATTR_LONG_NAME (FAT_ATTR_READ_ONLY | FAT_ATTR_HIDDEN | FAT_ATTR_SYSTEM | FAT_ATTR_VOLUME_ID)
#define FAT_ATTR_LONG_NAME_MASK (FAT_ATTR_READ_ONLY | FAT_ATTR_HIDDEN | FAT_ATTR_SYSTEM | FAT_ATTR_VOLUME_ID | FAT_ATTR_DIRECTORY | FAT_ATTR_ARCHIVE)

#define FAT_DIRECTORY_ENTRY_EMPTY 0xE5
#define FAT_DIRECTORY_ENTRY_EMPTY_LAST 0x00


struct FatLongFileNameEntry
{
    uint8_t ord;
    char name1[10];
    uint8_t attributes;
    uint8_t type;
    uint8_t checksum;
    char name2[12];
    uint16_t fstClusLo;
    char name3[4];
} PACKED;

#define FAT_DIRECTORY_ENTRY_LAST_LONG_NAME_ENTRY 0x40
#define FAT_DIRECTORY_ENTRY_LONG_NAME_ENTRY_MASK 0x1F

#define FAT_MAX_LONG_FILE_NAME_ENTRIES 20

#endif