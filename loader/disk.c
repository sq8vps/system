#include "disk.h"

#define MBR_SIGNATURE 0x1B8
#define MBR_FIRST_ENTRY 0x1BE
#define MBR_ENTRY_SIZE 16

#define MBR_ENTRY_TYPE_OFFSET 4
#define MBR_TYPE_EMPTY 0
#define MBR_TYPE_GPTPROT 0xEE
#define MBR_TYPE_FAT32 0x0C

#define MBR_ENTRY_LBA_OFFSET 0x08
#define MBR_ENTRY_SIZE_OFFSET 0x0C

void disk_init(void)
{
	for(uint16_t i = 0; i < DISK_TABLE_MAX_ENTRIES; i++)
	{
		diskTable[i].present = 0;
		diskTable[i].attr = 0;
		diskTable[i].ata = (void*)0;
		for(uint8_t j = 0; j < DISK_TABLE_MAX_PARTITIONS; j++)
		{
			diskTable[i].partitions[j].present = 0;
			diskTable[i].partitions[j].size = 0;
			diskTable[i].partitions[j].firstLba = 0;
			diskTable[i].partitions[j].type = TYPE_EMPTY;
		}
	}
}


error_t disk_getPartitions(Disk_s_t *disk)
{
	if(disk->present != 1)
		return DISK_NOT_PRESENT;



	uint8_t d[4096];
	error_t ret = disk_read(*disk, d, 0, 1); //read first sector
	if(ret != DISK_OK)
		return ret;

	if(d[MBR_FIRST_ENTRY + MBR_ENTRY_TYPE_OFFSET] == MBR_TYPE_GPTPROT) //GPT protective partition, this disk uses GPT
	{
#if __DEBUG > 1
		printf("Found GPT scheme, not implemented yet\n");
#endif
		return NOT_IMPLEMENTED;
	}

	//else it's MBR

	uint32_t signature = d[MBR_SIGNATURE] | d[MBR_SIGNATURE + 1] << 8 | d[MBR_SIGNATURE + 2] << 16 | d[MBR_SIGNATURE + 3] << 24;
	if(signature == 0 || signature == 0xFFFFFFFF) //MBR disk we accept should have some signature
	{
#if __DEBUG > 1
		printf("Disk is not partitioned\n");
#endif
		return DISK_NOT_PARTITIONED;
	}




	for(uint8_t i = 0; i < 4; i++)
	{
		uint8_t *partHeader = &(d[MBR_FIRST_ENTRY + i * MBR_ENTRY_SIZE]);
		uint8_t type = partHeader[MBR_ENTRY_TYPE_OFFSET];
		if(type == MBR_TYPE_EMPTY)
		{
			disk->partitions[i].present = 0;
		}
		else
		{

			disk->partitions[i].size = (uint32_t)partHeader[MBR_ENTRY_SIZE_OFFSET] | (uint32_t)partHeader[MBR_ENTRY_SIZE_OFFSET + 1] << 8 | (uint32_t)partHeader[MBR_ENTRY_SIZE_OFFSET + 2] << 16 | (uint32_t)partHeader[MBR_ENTRY_SIZE_OFFSET + 3] << 24;
			disk->partitions[i].firstLba = (uint32_t)partHeader[MBR_ENTRY_LBA_OFFSET] | (uint32_t)partHeader[MBR_ENTRY_LBA_OFFSET + 1] << 8 | (uint32_t)partHeader[MBR_ENTRY_LBA_OFFSET + 2] << 16 | (uint32_t)partHeader[MBR_ENTRY_LBA_OFFSET + 3] << 24;
			disk->partitions[i].present = 1;
#if __DEBUG > 1
		printf("Found MBR partition %d, LBA %u, size %u, type ", (int)i, (unsigned int)disk->partitions[i].firstLba, (unsigned int)disk->partitions[i].size);
#endif
			if(type == MBR_TYPE_FAT32)
			{
				disk->partitions[i].type = TYPE_FAT32;
#if __DEBUG > 1
				printf("FAT32\n");
#endif
			}
			else
			{
				disk->partitions[i].type = TYPE_UNKNOWN;
#if __DEBUG > 1
				printf("unknown\n");
#endif
			}

		}
	}

	return DISK_OK;
}

void disk_add(void *ata, uint8_t attr)
{
	uint16_t i = 0;
	for(; i < DISK_TABLE_MAX_ENTRIES; i++)
	{
		if(diskTable[i].present == 0) break; //this slot is free
	}
	if(i == (DISK_TABLE_MAX_ENTRIES)) return; //check if there is enough room for the next entry

	diskTable[i].present = 1;
	diskTable[i].ata = ata;
	diskTable[i].attr = attr;

	disk_getPartitions(&diskTable[i]);
}


/**
 * \brief Reads sectors from disk
 * \param disk diskTable entry. Disk must be present.
 * \param *buf Destination buffer, must be word aligned
 * \param lba Starting LBA (48-bit)
 * \param sec Sector count (higher than 0)
 * \return ATA_OK if successful, otherwise destination buffer must be invalidated
 */
error_t disk_read(Disk_s_t disk, uint8_t *dest, uint64_t lba, uint32_t sec)
{
	error_t ret = 0;
	if(disk.present != 1) return DISK_NOT_PRESENT;
	if(disk.attr & DISK_TABLE_ATTR_IDE_AHCI_BIT) //AHCI controller
	{

	}
	else //IDE controller
	{
		ret = ata_IDEreadWrite(*(AtaController_s_t*)(disk.ata), 0, (disk.attr & DISK_TABLE_ATTR_CHAN_BIT) > 0, (disk.attr & DISK_TABLE_ATTR_PRIMSEC_BIT) > 0, dest, lba, sec);
	}
	if(ret == ATA_OK) return DISK_OK;
	else return ret;
}

/**
 * \brief Writes sectors to disk
 * \param disk diskTable entry. Disk must be present.
 * \param *src Source buffer, must be word aligned
 * \param lba Starting LBA (48-bit)
 * \param sec Sector count (higher than 0)
 * \return ATA_OK if successful, otherwise destination sectors must be invalidated
 */
error_t disk_write(Disk_s_t disk, uint8_t *src, uint64_t lba, uint16_t sec)
{
	error_t ret = 0;
	if(disk.present != 1) return DISK_NOT_PRESENT;
	if(disk.attr & DISK_TABLE_ATTR_IDE_AHCI_BIT) //AHCI controller
	{

	}
	else //IDE controller
	{
		ret = ata_IDEreadWrite(*(AtaController_s_t*)(disk.ata), 1, (disk.attr & DISK_TABLE_ATTR_CHAN_BIT) > 0, (disk.attr & DISK_TABLE_ATTR_PRIMSEC_BIT) > 0, src, lba, sec);
	}
	if(ret == ATA_OK) return DISK_OK;
	else return ret;
}



error_t disk_readPartition(Disk_s_t disk, uint8_t partition, uint8_t *dest, uint64_t lba, uint16_t sec)
{
	if(disk.present != 1)
		return DISK_NOT_PRESENT;

	if(partition >= DISK_TABLE_MAX_PARTITIONS)
		return PARTITION_EMPTY;

	if(sec > (disk.partitions[partition].size - lba))
		return PARTITION_TOO_SMALL;

	return disk_read(disk, dest, disk.partitions[partition].firstLba + lba, sec);
}


Disk_params_s_t disk_getParams(Disk_s_t disk)
{
	Disk_params_s_t d;
	d.present = 0;
	d.sectorCount = 0;
	d.sectorSize = 0;
	if(disk.present == 0)
		return d;

	d.present = 1;
	if(disk.attr & DISK_TABLE_ATTR_IDE_AHCI_BIT == 0)
	{
		d.sectorSize = ((AtaController_s_t*)(disk.ata))->disk[disk.attr & DISK_TABLE_ATTR_PRIMSEC_BIT][disk.attr & DISK_TABLE_ATTR_CHAN_BIT].sectorSize;
		d.sectorCount = ((AtaController_s_t*)(disk.ata))->disk[disk.attr & DISK_TABLE_ATTR_PRIMSEC_BIT][disk.attr & DISK_TABLE_ATTR_CHAN_BIT].sectorCount;
	}
	return d;
}


