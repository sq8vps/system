#include "disk.h"
#include "mm.h"

#define MBR_SIGNATURE 0x1B8
#define MBR_FIRST_ENTRY 0x1BE
#define MBR_ENTRY_SIZE 16

#define MBR_ENTRY_TYPE_OFFSET 4
#define MBR_TYPE_EMPTY 0
#define MBR_TYPE_GPTPROT 0xEE
#define MBR_TYPE_FAT32 0x0C

#define MBR_ENTRY_LBA_OFFSET 0x08
#define MBR_ENTRY_SIZE_OFFSET 0x0C

#define BUFFER_VADDR 0x100000
uint32_t buffer_pAddr = 0;

error_t Disk_init(void)
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
	error_t ret = Mm_allocateContiguous(DISK_BUFFER_SIZE / MM_PAGE_SIZE, BUFFER_VADDR, MM_PAGE_FLAG_WRITABLE, 65536 / MM_PAGE_SIZE); //allocate disk buffer that is aligned to 64 KiB (ATA requirement)
	if(ret != OK)
		return ret;

	return Mm_getPhysAddr(BUFFER_VADDR, &buffer_pAddr);
}


error_t Disk_getPartitions(Disk_s_t *disk)
{
	if(disk->present != 1)
		return DISK_NOT_PRESENT;



	uint8_t d[512];
	error_t ret = Disk_read(*disk, d, 0, 0, 512); //read first sector
	if(ret != OK)
	{
		return ret;
	}

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

	return OK;
}

void Disk_add(void *ata, uint8_t attr)
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

	Disk_getPartitions(&diskTable[i]);
}


/**
 * \brief Reads sectors from disk
 * \param disk diskTable entry. Disk must be present.
 * \param *buf Destination buffer, must be word aligned
 * \param lba Starting LBA (48-bit)
 * \param offset Starting offset in bytes
 * \param size Byte count
 * \return OK if successful, otherwise destination buffer must be invalidated
 */
error_t Disk_read(Disk_s_t disk, uint8_t *dest, uint64_t lba, uint32_t offset, uint32_t size)
{
	error_t ret = OK;
	if(disk.present != 1)
		return DISK_NOT_PRESENT;
	if(disk.attr & DISK_TABLE_ATTR_IDE_AHCI_BIT) //AHCI controller
	{

	}
	else //IDE controller
	{
		uint16_t sectorSize = ((AtaController_s_t*)(disk.ata))->disk[(disk.attr & DISK_TABLE_ATTR_CHAN_BIT) > 0][(disk.attr & DISK_TABLE_ATTR_PRIMSEC_BIT) > 0].sectorSize;

		uint32_t destIdx = 0;

		for(uint16_t i = 0; i < (size / DISK_BUFFER_SIZE); i++)
		{
			uint16_t sec = DISK_BUFFER_SIZE / sectorSize; //calculate sector count

			ret = ata_IDEreadWrite(*(AtaController_s_t*)(disk.ata), 0, (disk.attr & DISK_TABLE_ATTR_CHAN_BIT) > 0, (disk.attr & DISK_TABLE_ATTR_PRIMSEC_BIT) > 0, (uint8_t*)buffer_pAddr, lba, sec); //read to buffer

			lba += sec;

			if(ret == OK)
			{
				for(uint32_t k = offset; k < DISK_BUFFER_SIZE; k++)
				{
					dest[destIdx++] = ((uint8_t*)BUFFER_VADDR)[k]; //copy necessary bytes to destination buffer
				}
			}
			else
				return ret;

			offset = 0;
		}

		size %= DISK_BUFFER_SIZE;

		if(size)
		{
			uint16_t sec = size / sectorSize; //calculate sector count
			ret = ata_IDEreadWrite(*(AtaController_s_t*)(disk.ata), 0, (disk.attr & DISK_TABLE_ATTR_CHAN_BIT) > 0, (disk.attr & DISK_TABLE_ATTR_PRIMSEC_BIT) > 0, (uint8_t*)buffer_pAddr, lba, sec +
					((size % sectorSize) ? 1 : 0)); //read to buffer

			if(ret == OK)
			{
				for(uint32_t k = 0; k < size; k++)
				{
					dest[destIdx++] = ((uint8_t*)BUFFER_VADDR)[k + offset]; //copy necessary bytes to destination buffer
				}
			}
			else
				return ret;
		}
	}


	return ret;
}

/**
 * \brief Writes sectors to disk
 * \param disk diskTable entry. Disk must be present.
 * \param *src Source buffer, must be word aligned
 * \param lba Starting LBA (48-bit)
 * \param size Byte count
 * \return OK if successful, otherwise destination sectors must be invalidated
 */
error_t Disk_write(Disk_s_t disk, uint8_t *src, uint64_t lba, uint32_t size)
{
	return NOT_IMPLEMENTED;

	error_t ret = OK;
	if(disk.present != 1) return DISK_NOT_PRESENT;
	if(disk.attr & DISK_TABLE_ATTR_IDE_AHCI_BIT) //AHCI controller
	{

	}
	else //IDE controller
	{

		uint32_t srcIdx = 0;

		for(uint16_t i = 0; i < (size / DISK_BUFFER_SIZE); i++)
		{
			for(uint32_t k = 0; k < DISK_BUFFER_SIZE; k++)
			{
				((uint8_t*)BUFFER_VADDR)[k] = src[srcIdx++]; //copy necessary bytes from source buffer
			}

			uint16_t sec = DISK_BUFFER_SIZE / ((AtaController_s_t*)(disk.ata))->disk[(disk.attr & DISK_TABLE_ATTR_CHAN_BIT) > 0][(disk.attr & DISK_TABLE_ATTR_PRIMSEC_BIT) > 0].sectorSize; //calculate sector count

			ret = ata_IDEreadWrite(*(AtaController_s_t*)(disk.ata), 1, (disk.attr & DISK_TABLE_ATTR_CHAN_BIT) > 0, (disk.attr & DISK_TABLE_ATTR_PRIMSEC_BIT) > 0, (uint8_t*)buffer_pAddr, lba, sec); //store to disk

			lba += sec;

			if(ret != OK)
				return ret;
		}

		if(size % DISK_BUFFER_SIZE)
		{
			for(uint32_t k = 0; k < size % DISK_BUFFER_SIZE; k++)
			{
				((uint8_t*)BUFFER_VADDR)[k] = src[srcIdx++]; //copy necessary bytes from source buffer
			}

			ret = ata_IDEreadWrite(*(AtaController_s_t*)(disk.ata), 1, (disk.attr & DISK_TABLE_ATTR_CHAN_BIT) > 0, (disk.attr & DISK_TABLE_ATTR_PRIMSEC_BIT) > 0, (uint8_t*)buffer_pAddr, lba, 1); //store to disk

			if(ret != OK)
				return ret;
		}
	}

	return ret;
}



error_t Disk_readPartition(Disk_s_t disk, uint8_t partition, uint8_t *dest, uint64_t lba, uint32_t offset, uint32_t size)
{
	if(disk.present != 1)
		return DISK_NOT_PRESENT;

	if(partition >= DISK_TABLE_MAX_PARTITIONS)
		return PARTITION_EMPTY;

	//if(sec > (disk.partitions[partition].size - lba))
	//	return PARTITION_TOO_SMALL;

	return Disk_read(disk, dest, disk.partitions[partition].firstLba + lba, offset, size);
}


Disk_params_s_t Disk_getParams(Disk_s_t disk)
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
