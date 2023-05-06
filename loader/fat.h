/*
 * fat.h
 *
 *  Created on: 30.12.2020
 *      Author: Piotr
 */

#ifndef LOADER_FAT_H_
#define LOADER_FAT_H_

#include "defines.h"
#include <stdint.h>
#include "disk.h"

#define FAT32_BPS_OFFSET 0xB //bytes per sector offset
#define FAT32_SPC_OFFSET 0xD //sectors per cluster offset
#define FAT32_RSVD_OFFSET 0xE //reserved sector count offset
#define FAT32_FATCNT_OFFSET 0x10 //number of FATs offset
#define FAT32_FATSIZE_OFFSET 0x24 //FAT size offset
#define FAT32_ROOT_OFFSET 0x2c //root cluster number offset

struct Fat32Header
{
	uint16_t bytesPerSector; //bytes per disk sector
	uint8_t sectorPerCluster; //cluster size in unit of disk sectors
	uint16_t reservedSectors; //number of reserved sectors between header and first FAT
	uint8_t fatCount; //number of FATs
	uint32_t fatSize; //FAT size in unit of sectors
	uint32_t rootCluster; //location of root cluster (cluster number)

	//these are not included in FAT32 header, but are derived from them
	uint32_t firstFatAddr; //first FAT address in units of sectors
	uint32_t secondFatAddr;
	uint32_t rootClusterAddr; //root cluster address in units of sectors
	uint32_t bytesPerCluster; //bytes in one cluster
};

typedef struct
{
	struct Fat32Header header;
	Disk_s_t *disk;
	uint8_t partition;
	uint8_t initialized;
	uint32_t currentCluster;
} Fat32_s_t;


extern Fat32_s_t fatDisk;

error_t Fat_init(Disk_s_t *disk, uint8_t partition);

error_t Fat_changeDir(Fat32_s_t *fat, uint8_t *path);
error_t Fat_changeDirN(Fat32_s_t *fat, uint8_t *path, uint16_t pathLen);

error_t Fat_getFileSize(Fat32_s_t *fat, uint8_t *path, uint32_t *size);
error_t Fat_readWholeFile(Fat32_s_t *fat, uint8_t *path, uint8_t *dest, uint32_t *outSize);
error_t Fat_readFile(Fat32_s_t *fat, uint8_t *path, uint32_t start, uint32_t size, uint8_t *dest);
#endif /* LOADER_FAT_H_ */
