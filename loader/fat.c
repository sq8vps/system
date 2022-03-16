#include "fat.h"

#include "disp.h"


#define FAT_ENTRY_SHIFT_NAME 0x00
#define FAT_ENTRY_SHIFT_EXTENSION 0x08
#define FAT_ENTRY_SHIFT_ATTR 0x0B
#define FAT_ENTRY_SHIFT_CLUSTER_HI 0x14
#define FAT_ENTRY_SHIFT_CLUSTER_LO 0x1A
#define FAT_ENTRY_SHIFT_SIZE 0x1C

#define FAT_ENTRY_ETTR_READONLY 0x01
#define FAT_ENTRY_ATTR_HIDDEN 0x02
#define FAT_ENTRY_ATTR_SYSTEM 0x04
#define FAT_ENTRY_ATTR_VOLUME_LABEL 0x08
#define FAT_ENTRY_ATTR_SUBDIR 0x10
#define FAT_ENTRY_ATTR_ARCHIVE 0x20




enum Fat_file_type
{
	FAT_FILE,
	FAT_DIRECTORY,
};

uint8_t buf[4096] __attribute__ ((aligned(8)));

error_t Fat_init(Disk_s_t *disk, uint8_t partition)
{
	if(disk->present == 0)
		return FAT_DISK_NOT_PRESENT;
	if(disk->partitions[partition].present == 0 || disk->partitions[partition].type != TYPE_FAT32)
		return FAT_NOT_FAT;

	error_t ret = disk_readPartition(*disk, partition, buf, 0, 1); //read first partition sector
	if(ret != DISK_OK)
		return ret;



	fatDisk.disk = disk;
	fatDisk.partition = partition;
	fatDisk.header.bytesPerSector = bytesToUint16LE(&buf[FAT32_BPS_OFFSET]); //store data from BIOS parameter block
	fatDisk.header.sectorPerCluster = buf[FAT32_SPC_OFFSET];
	fatDisk.header.reservedSectors = bytesToUint16LE(&buf[FAT32_RSVD_OFFSET]);
	fatDisk.header.fatCount = buf[FAT32_FATCNT_OFFSET];
	fatDisk.header.fatSize = bytesToUint32LE(&buf[FAT32_FATSIZE_OFFSET]);
	fatDisk.header.rootCluster = bytesToUint16LE(&buf[FAT32_ROOT_OFFSET]);

	fatDisk.header.firstFatAddr = fatDisk.header.reservedSectors;
	fatDisk.header.secondFatAddr = fatDisk.header.firstFatAddr + fatDisk.header.fatSize;
	fatDisk.header.rootClusterAddr = fatDisk.header.secondFatAddr + fatDisk.header.fatSize + (fatDisk.header.rootCluster - 2) * fatDisk.header.sectorPerCluster;
	fatDisk.currentCluster = fatDisk.header.rootCluster;
	fatDisk.header.bytesPerCluster = fatDisk.header.bytesPerSector * fatDisk.header.sectorPerCluster;

	fatDisk.initialized = 1;
}

error_t fat_getNextCluster(Fat32_s_t *fat, uint32_t currentCluster, uint32_t *nextCluster)
{
	if(currentCluster < 2)
		return FAT_INVALID_CLUSTER;

	//there are 4 bytes per FAT entry, so one sector contains entries_in_sector=sector_size/4 FAT entries, each corresponding to one cluster (!)
	//first we need to calculate in which sector the appropriate entry resides, so sector=floor(cluster_number/entries_in_sector),
	//that gives sector=floor(cluster_number*4/sector_size)
	//the byte number in sector is equal to cluster_number mod entries_in_sector
	error_t ret = disk_readPartition(*(fat->disk), fat->partition, buf, fat->header.firstFatAddr + (currentCluster * 4 / fat->header.bytesPerSector), 1); //read appropriate FAT sector
	if(ret != DISK_OK)
		return ret;


	currentCluster %= (fat->header.bytesPerSector / 4); //now currentCluster stores the byte index
	*nextCluster = (uint32_t)buf[currentCluster * 4];
	*nextCluster |= (uint32_t)buf[currentCluster * 4 + 1] << 8;
	*nextCluster |= (uint32_t)buf[currentCluster * 4 + 2] << 16;
	*nextCluster |= (uint32_t)buf[currentCluster * 4 + 3] << 24;

	if((*nextCluster >= 0xFFFFFF8) || (*nextCluster & 0xFFFFFFF <= 1)) //end of chain
		return FAT_EOC;
	else if(*nextCluster & 0xFFFFFFF == 0xFFFFFF7)
		return FAT_BROKEN_CLUSTER;

	return FAT_OK;
}

error_t fat_selectFile(Fat32_s_t *fat, uint32_t cluster, uint8_t *name, enum Fat_file_type type, uint32_t *outCluster, uint32_t *outSize)
{
	error_t ret = disk_readPartition(*(fat->disk), fat->partition, buf, fat->header.rootClusterAddr + (cluster - 2) * fat->header.sectorPerCluster, fat->header.sectorPerCluster); //read cluster
	if(ret != DISK_OK)
		return ret;

	uint16_t i = 0; //byte iterator
	while(1) //loop for directory entries
	{
		if(buf[i] == 0) //first byte is 0, the entry is empty and subsequent entries are assumed to be empty, too
		{
#if __DEBUG > 0
			printf("File/directory \"%s\" does not exist\n", name);
#endif
			return FAT_NOT_FOUND;
		}
		if(buf[i + FAT_ENTRY_SHIFT_ATTR] != 0x0F) //this seems to be a normal entry
		{
			uint8_t err = 0;

				if((i >= 32) && (buf[i - 32 + FAT_ENTRY_SHIFT_ATTR] == 0x0F)) //there is a preceding Long File Name (LFN) entry
				{
					/**
					 * TODO: this implementation does not support LFN when LFN entries are split across different clusters
					 */
					uint8_t readName[261]; //Long file name is at most 260 characters long
					uint16_t readNameIdx = 0;
					for(uint16_t j = 1; j < 14; j++) //at most 13 LFN entries
					{
						for(uint8_t k = 0; k < 10; k += 2) //five characters
						{
							readName[readNameIdx] = buf[i - 32 * j + k + 1]; //store character
							if(readName[readNameIdx] == 0) //end of name
								break;

							readNameIdx++;
						}
						for(uint8_t k = 0; k < 12; k += 2) //six characters
						{
							readName[readNameIdx] = buf[i - 32 * j + k + 14]; //store character
							if(readName[readNameIdx] == 0) //end of name
								break;

							readNameIdx++;
						}
						for(uint8_t k = 0; k < 4; k += 2) //two characters
						{
							readName[readNameIdx] = buf[i - 32 * j + k + 28]; //store character
							if(readName[readNameIdx] == 0) //end of name
								break;

							readNameIdx++;
						}
						if(buf[i - 32 * j] & 0x40) //last LFN entry
							break;
					}
					readName[readNameIdx] = 0;
					if(strcmp(name, readName) != 0) //file name is not matching
						err = 1;
				}
				else //no long file name entry, parse 8.3 file name
				{
					uint8_t readName[14];
					uint8_t readNameIdx = 0;
					for(uint8_t j = 0; j < 8; j++) //8. part
					{
						readName[readNameIdx++] = buf[i + j];
					}

					for(uint8_t j = 7; j > 0; j--) //trim spaces
					{
						if(readName[readNameIdx - 1] == ' ')
							readNameIdx--;
						else
							break;
					}

					if(buf[i + FAT_ENTRY_SHIFT_EXTENSION] != ' ')
						readName[readNameIdx++] = '.'; //add dot

					for(uint8_t j = 0; j < 3; j++) //.3 part
					{
						if(buf[i + FAT_ENTRY_SHIFT_EXTENSION + j] == ' ') //end of file name
							break;

						readName[readNameIdx++] = buf[i + FAT_ENTRY_SHIFT_EXTENSION + j];
					}
					readName[readNameIdx++] = 0;

					if(strlen(readName) == strlen(name)) //file name length is matching
					{
						for(uint8_t j = 0; j < strlen(name); j++)
						{
							if(!((readName[j] == name[j]) || (readName[j] == (name[j] - 32))))
								err = 1;
						}
					}
					else
						err = 1;
				}

			if(err == 0) //file found
			{
				if(buf[i + FAT_ENTRY_SHIFT_ATTR] & FAT_ENTRY_ATTR_SUBDIR)
				{
					if(type == FAT_FILE)
					{
#if __DEBUG > 0
						printf("\"%s\" is a directory\n", name);
						return FAT_NOT_FOUND;
#endif
					}
				}
				else
				{
					if(type == FAT_DIRECTORY)
					{
#if __DEBUG > 0
						printf("\"%s\" is a not directory\n", name);
						return FAT_NOT_FOUND;
#endif
					}
				}
				*outCluster = (uint32_t)buf[i + FAT_ENTRY_SHIFT_CLUSTER_HI + 1] << 24;
				*outCluster |= (uint32_t)buf[i + FAT_ENTRY_SHIFT_CLUSTER_HI] << 16;
				*outCluster |= (uint32_t)buf[i + FAT_ENTRY_SHIFT_CLUSTER_LO + 1] << 8;
				*outCluster |= (uint32_t)buf[i + FAT_ENTRY_SHIFT_CLUSTER_LO];

				*outSize = (uint32_t)buf[i + FAT_ENTRY_SHIFT_SIZE + 3] << 24;
				*outSize |= (uint32_t)buf[i + FAT_ENTRY_SHIFT_SIZE + 2] << 16;
				*outSize |= (uint32_t)buf[i + FAT_ENTRY_SHIFT_SIZE + 1] << 8;
				*outSize |= (uint32_t)buf[i + FAT_ENTRY_SHIFT_SIZE];

				return FAT_OK;
			}

		}



		i += 32;
		if(i >= fat->header.bytesPerCluster) //end of cluster
		{
			ret = fat_getNextCluster(fat, cluster, &cluster); //check next cluster
			if(ret == FAT_OK)
			{
				ret = disk_readPartition(*(fat->disk), fat->partition, buf, fat->header.rootClusterAddr + (cluster - 2) * fat->header.sectorPerCluster, fat->header.sectorPerCluster); //read cluster
				if(ret != DISK_OK)
					return ret;
			}
			else if(ret == FAT_EOC) //end of chain, file not found in this directory
			{
#if __DEBUG > 0
				printf("File/directory \"%s\" does not exist\n", name);
#endif
				return FAT_NOT_FOUND;
			}
			else
			{
#if __DEBUG > 0
				printf("FAT32 failure\n");
#endif
				return FAT_NOT_FOUND;
			}
			i = 0;
		}
	}
	return FAT_NOT_FOUND;
}

error_t Fat_changeDir(Fat32_s_t *fat, uint8_t *path)
{
	if(fat->initialized == 0)
		return FAT_NOT_INITIALIZED;

	uint32_t clusterNo = 2;

	uint16_t idx = 0;

	if(path[0] == '/') //starting from root directory
	{
		clusterNo = fat->header.rootCluster;
		idx = 1;
	}
	else //starting from current directory
		clusterNo = fat->currentCluster;

	uint8_t fileName[261];
	uint16_t fileNameIdx = 0;

	for(; idx < strlen(path); idx++)
	{
		if((path[idx + 1] == 0) && (path[idx] != '/'))
			fileName[fileNameIdx++] = path[idx];

		if((path[idx] == '/') || (path[idx + 1] == 0)) //separator
		{
			fileName[fileNameIdx++] = 0;

			uint32_t fileCluster, fileSize;
			if(fat_selectFile(fat, clusterNo, fileName, FAT_DIRECTORY, &fileCluster, &fileSize) == FAT_OK)
			{
				fat->currentCluster = fileCluster;
				clusterNo = fileCluster;
			}
			else
			{
				return FAT_NOT_FOUND;
			}
			fileNameIdx = 0;
		}
		else
			fileName[fileNameIdx++] = path[idx];
	}
	return FAT_OK;
}

error_t Fat_getFileSize(Fat32_s_t *fat, uint8_t *name, uint32_t *size)
{
	if(fat->initialized == 0)
		return FAT_NOT_INITIALIZED;

	uint32_t dummy;
	if(fat_selectFile(fat, fat->currentCluster, name, FAT_FILE, &dummy, size) == FAT_OK)
	{
		return FAT_OK;
	}
	else
	{
		return FAT_NOT_FOUND;
	}
}

error_t Fat_readWholeFile(Fat32_s_t *fat, uint8_t *name, uint8_t *dest, uint32_t *outSize)
{
	if(fat->initialized == 0)
		return FAT_NOT_INITIALIZED;

	error_t ret = FAT_OK;


	uint32_t cluster;
	ret = fat_selectFile(fat, fat->currentCluster, name, FAT_FILE, &cluster, outSize); //select (find) file
	if(ret != FAT_OK)
		return ret;

	uint16_t i = 0;
	for(; i < (*outSize / fat->header.bytesPerCluster); i++) //loop for whole clusters
	{
		ret = disk_readPartition(*(fat->disk), fat->partition, dest + i * fat->header.bytesPerCluster, fat->header.rootClusterAddr + (cluster - 2) * fat->header.sectorPerCluster, fat->header.sectorPerCluster); //read cluster
		if(ret != DISK_OK)
			return ret;

		ret = fat_getNextCluster(fat, cluster, &cluster); //check next cluster
		if(ret != FAT_OK)
			return ret;

	}
	if(*outSize % fat->header.bytesPerCluster) //remaining bytes
	{
		ret = disk_readPartition(*(fat->disk), fat->partition, dest + i * fat->header.bytesPerCluster, fat->header.rootClusterAddr + (cluster - 2) * fat->header.sectorPerCluster,
				(*outSize % fat->header.bytesPerCluster) / fat->header.bytesPerSector + (*outSize % fat->header.bytesPerSector) ? 1 : 0); //read cluster
		if(ret != DISK_OK)
			return ret;
	}

	return FAT_OK;
}


error_t Fat_readFile(Fat32_s_t *fat, uint8_t *name, uint32_t start, uint32_t end, uint8_t *dest)
{
	if(fat->initialized == 0)
		return FAT_NOT_INITIALIZED;

	if(start > end)
		return FAT_INVALID_VAL;

	error_t ret = FAT_OK;


	uint32_t cluster, size;
	ret = fat_selectFile(fat, fat->currentCluster, name, FAT_FILE, &cluster, &size); //select (find) file
	if(ret != FAT_OK)
		return ret;

	for(uint16_t i = 0; i < (start / fat->header.bytesPerCluster); i++)
	{
		ret = fat_getNextCluster(fat, cluster, &cluster); //look for the first needed cluster
		if(ret != FAT_OK)
			return ret;
	}

	uint16_t i = 0;
	for(; i < ((end - start + 1) / fat->header.bytesPerCluster); i++) //loop for whole clusters
	{
		ret = disk_readPartition(*(fat->disk), fat->partition, dest + i * fat->header.bytesPerCluster, fat->header.rootClusterAddr + (cluster - 2) * fat->header.sectorPerCluster, fat->header.sectorPerCluster); //read cluster
		if(ret != DISK_OK)
			return ret;

		ret = fat_getNextCluster(fat, cluster, &cluster); //check next cluster
		if(ret != FAT_OK)
			return ret;

	}
	if((end - start + 1) % fat->header.bytesPerCluster) //remaining bytes
	{
		ret = disk_readPartition(*(fat->disk), fat->partition, dest + i * fat->header.bytesPerCluster, fat->header.rootClusterAddr + (cluster - 2) * fat->header.sectorPerCluster,
				((end - start + 1) % fat->header.bytesPerCluster) / fat->header.bytesPerSector + ((end - start + 1) % fat->header.bytesPerSector) ? 1 : 0); //read cluster
		if(ret != DISK_OK)
			return ret;
	}

	return FAT_OK;
}
