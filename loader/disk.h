/*
 * disk.h
 *
 *  Created on: 31.12.2020
 *      Author: Piotr
 */

#ifndef LOADER_DISK_H_
#define LOADER_DISK_H_

#include <stdint.h>
#include "ata.h"
#include "disp.h"

#define DISK_TABLE_MAX_ENTRIES (ATA_MAX_CONTROLLERS * 4)
#define DISK_TABLE_MAX_PARTITIONS 4

#define DISK_TABLE_ATTR_CHAN_BIT 2
#define DISK_TABLE_ATTR_PRIMSEC_BIT 4
#define DISK_TABLE_ATTR_IDE_AHCI_BIT 1
#define DISK_TABLE_ATTR_MBR_GPT_BIT 64

#define DISK_BUFFER_SIZE 1048576 //disk buffer size in bytes (1 MiB by default)

enum Partition_type
{
	TYPE_EMPTY,
	TYPE_UNKNOWN,
	TYPE_FAT32,
};

/**
 * \brief Disk structure
 */
typedef struct
{
	uint8_t present; //1 if disk is present, 0 if not (and the other structure members are invalid)
	void *ata; //pointer to the ATA controller or AHCI controller structure
	uint8_t attr; //disk attributes, bit 0 - mode: 0 for IDE mode (native ATA or compatibility mode SATA), 1 for SATA AHCI mode
				  //if bit0==0 (IDE): bit 1 - IDE channel (0 for primary, 1 for secondary), bit 2 - channel slot (0 for master, 1 for slave)
				  //if bit0==1 (AHCI): bits 1:5 - AHCI device/port number (0-31)
				  //bit 6 - partitioning scheme: 0 for MBR, 1 for GPT
	struct
	{
		uint8_t present;
		uint64_t firstLba;
		uint64_t size;
		enum Partition_type type;
	} partitions[DISK_TABLE_MAX_PARTITIONS];
} Disk_s_t;

extern Disk_s_t diskTable[DISK_TABLE_MAX_ENTRIES]; /** Disk table */

typedef struct
{
	uint8_t present;
	uint16_t sectorSize;
	uint64_t sectorCount;
} Disk_params_s_t;

/**
 * @brief Initialize PCI/PCI-E IDE/AHCI controllers and disks
 * @attention Paging MUST be enabled
 */
error_t Disk_init(void);

/**
 * \brief Adds disk to the list
 * \param *ata IDE or AHCI controller structure
 * \param attr Attributes as specified at the top of this file
 */
void Disk_add(void *ata, uint8_t attr);


/**
 * \brief Reads sectors from disk
 * \param disk diskTable entry. Disk must be present.
 * \param *buf Destination buffer, must be word aligned
 * \param lba Starting LBA (48-bit)
 * \param offset Starting offset in bytes
 * \param size Byte count
 * \return OK if successful, otherwise destination buffer must be invalidated
 */
error_t Disk_read(Disk_s_t disk, uint8_t *dest, uint64_t lba, uint32_t offset, uint32_t size);

/**
 * \brief Writes sectors to disk
 * \param disk diskTable entry. Disk must be present.
 * \param *src Source buffer, must be word aligned
 * \param lba Starting LBA (48-bit)
 * \param size Byte count
 * \return OK if successful, otherwise destination sectors must be invalidated
 */
error_t Disk_write(Disk_s_t disk, uint8_t *src, uint64_t lba, uint32_t size);

/**
 * \brief Reads sectors from specified partition
 * \param disk diskTable entry. Disk must be present.
 * \param partition Partition number starting from 0. Partition must be present.
 * \param *buf Destination buffer, must be word aligned
 * \param lba Starting LBA (48-bit) in respect to partition start (LBA 0 is the first sector of the partition)
 * \param offset Starting offset in bytes
 * \param size Byte count
 * \return DISK_OK if successful, otherwise the destination buffer must be invalidated
 */
error_t Disk_readPartition(Disk_s_t disk, uint8_t partition, uint8_t *dest, uint64_t lba, uint32_t offset, uint32_t size);

/**
 * \brief Get general disk parameters
 * \param disk diskTable entry
 * \return Filled Disk_params_s_t structure
 */
Disk_params_s_t Disk_getParams(Disk_s_t disk);

#endif /* LOADER_DISK_H_ */
