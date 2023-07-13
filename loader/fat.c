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


Fat32_s_t fatDisk;

enum Fat_file_type
{
	FAT_FILE,
	FAT_DIRECTORY,
};

static uint8_t buf[8192];

error_t Fat_init(Disk_s_t *disk, uint8_t partition)
{
	if(disk->present == 0)
		return FAT_DISK_NOT_PRESENT;
	if(disk->partitions[partition].present == 0 || disk->partitions[partition].type != TYPE_FAT32)
		return FAT_NOT_FAT;

	error_t ret = Disk_readPartition(*disk, partition, buf, 0, 0, 512); //read first partition sector
	if(ret != OK)
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

	return OK;
}

static error_t fat_getNextCluster(Fat32_s_t *fat, uint32_t currentCluster, uint32_t *nextCluster)
{
	if(currentCluster < 2)
		return FAT_INVALID_CLUSTER;

	//there are 4 bytes per FAT entry, so one sector contains entries_in_sector=sector_size/4 FAT entries, each corresponding to one cluster (!)
	//first we need to calculate in which sector the appropriate entry resides, so sector=floor(cluster_number/entries_in_sector),
	//that gives sector=floor(cluster_number*4/sector_size)
	//the byte number in sector is equal to cluster_number mod entries_in_sector
	error_t ret = Disk_readPartition(*(fat->disk), fat->partition, buf, fat->header.firstFatAddr + (currentCluster * 4 / fat->header.bytesPerSector), 0, 512); //read appropriate FAT sector
	if(ret != OK)
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

	return OK;
}

static error_t fat_goToFile(Fat32_s_t *fat, uint8_t *path, uint8_t *name)
{
	int32_t t = strlen(path) - 1;
	error_t ret = OK;
	while((path[t] != '/') && (t >= 0)) //look for last separator
	{
		t--;
	}
	if(t >= 0)
	{
		ret = Fat_changeDirN(fat, path, t + 1);
	}
	name = path + t + 1;
	return ret;
}

static error_t fat_selectFile(Fat32_s_t *fat, uint32_t cluster, uint8_t *name, enum Fat_file_type type, uint32_t *outCluster, uint32_t *outSize)
{
	error_t ret = Disk_readPartition(*(fat->disk), fat->partition, buf, fat->header.rootClusterAddr + (cluster - 2) * fat->header.sectorPerCluster, 0, fat->header.bytesPerCluster); //read cluster
	if(ret != OK)
		return ret;
	
	uint16_t i = 0; //byte iterator
	while(1) //loop for directory entries
	{
		if(buf[i] == 0) //first byte is 0, the entry is empty and subsequent entries are assumed to be empty, too
		{
#if __DEBUG > 0
			printf("File/directory \"%s\" does not exist (empty entry)\n", name);
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

				return OK;
			}

		}



		i += 32;
		if(i >= fat->header.bytesPerCluster) //end of cluster
		{
			ret = fat_getNextCluster(fat, cluster, &cluster); //check next cluster
			if(ret == OK)
			{
				ret = Disk_readPartition(*(fat->disk), fat->partition, buf, fat->header.rootClusterAddr + (cluster - 2) * fat->header.sectorPerCluster, 0, fat->header.bytesPerCluster); //read cluster
				if(ret != OK)
					return ret;
			}
			else if(ret == FAT_EOC) //end of chain, file not found in this directory
			{
#if __DEBUG > 0
				printf("File/directory \"%s\" does not exist (EOC)\n", name);
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
	return Fat_changeDirN(fat, path, strlen(path));
}

error_t Fat_changeDirN(Fat32_s_t *fat, uint8_t *path, uint16_t pathLen)
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

	for(; idx < pathLen; idx++)
	{
		if((path[idx + 1] == 0) && (path[idx] != '/'))
			fileName[fileNameIdx++] = path[idx];

		if((path[idx] == '/') || (path[idx + 1] == 0)) //separator
		{
			fileName[fileNameIdx++] = 0;

			uint32_t fileCluster, fileSize;
			if(fat_selectFile(fat, clusterNo, fileName, FAT_DIRECTORY, &fileCluster, &fileSize) == OK)
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
	return OK;
}

error_t Fat_getFileSize(Fat32_s_t *fat, uint8_t *path, uint32_t *size)
{
	if(fat->initialized == 0)
		return FAT_NOT_INITIALIZED;

	uint32_t dummy;
	if(fat_selectFile(fat, fat->currentCluster, path, FAT_FILE, &dummy, size) == OK)
	{
		return OK;
	}
	else
	{
		return FAT_NOT_FOUND;
	}
}

error_t Fat_readWholeFile(Fat32_s_t *fat, uint8_t *path, uint8_t *dest, uint32_t *outSize)
{
	if(fat->initialized == 0)
		return FAT_NOT_INITIALIZED;

	error_t ret = OK;


	uint32_t cluster;
	ret = fat_selectFile(fat, fat->currentCluster, path, FAT_FILE, &cluster, outSize); //select (find) file
	if(ret != OK)
		return ret;

	uint16_t i = 0;
	for(; i < (*outSize / fat->header.bytesPerCluster); i++) //loop for whole clusters
	{
		ret = Disk_readPartition(*(fat->disk), fat->partition, dest + i * fat->header.bytesPerCluster, fat->header.rootClusterAddr + (cluster - 2) * fat->header.sectorPerCluster, 0, fat->header.bytesPerCluster); //read cluster
		if(ret != OK)
			return ret;

		ret = fat_getNextCluster(fat, cluster, &cluster); //check next cluster
		if(ret != OK)
			return ret;

	}
	if(*outSize % fat->header.bytesPerCluster) //remaining bytes
	{
		ret = Disk_readPartition(*(fat->disk), fat->partition, dest + i * fat->header.bytesPerCluster, fat->header.rootClusterAddr + (cluster - 2) * fat->header.sectorPerCluster, 0, *outSize % fat->header.bytesPerCluster); //read remaining bytes
		if(ret != OK)
			return ret;
	}

	return OK;
}


error_t Fat_readFile(Fat32_s_t *fat, uint8_t *path, uint32_t start, uint32_t size, uint8_t *dest)
{
	if(fat->initialized == 0)
		return FAT_NOT_INITIALIZED;

	if(size == 0)
		return OK;

	error_t ret = OK;


	uint32_t cluster, fileSize;
	ret = fat_selectFile(fat, fat->currentCluster, path, FAT_FILE, &cluster, &fileSize); //select (find) file
	if(ret != OK)
		return ret;

	if(size == 0)
		size = fileSize;

	while(start >= fat->header.bytesPerCluster) //start from appropriate cluster
	{
		ret = fat_getNextCluster(fat, cluster, &cluster); //look for the next cluster
		if(ret != OK)
			return ret;

		start -= fat->header.bytesPerCluster;
	}

	uint16_t i = 0;
	for(; i < (size / fat->header.bytesPerCluster); i++) //loop for whole clusters
	{
		ret = Disk_readPartition(*(fat->disk), fat->partition, dest + i * fat->header.bytesPerCluster, fat->header.rootClusterAddr + (cluster - 2) * fat->header.sectorPerCluster, start, fat->header.bytesPerCluster); //read cluster
		if(ret != OK)
			return ret;

		ret = fat_getNextCluster(fat, cluster, &cluster); //check next cluster
		if(ret != OK)
			return ret;

		start = 0;

	}
	if(size % fat->header.bytesPerCluster) //remaining bytes
	{
		ret = Disk_readPartition(*(fat->disk), fat->partition, dest + i * fat->header.bytesPerCluster, fat->header.rootClusterAddr + (cluster - 2) * fat->header.sectorPerCluster, start, size % fat->header.bytesPerCluster); //read remaining bytes
		if(ret != OK)
			return ret;
	}

	return OK;
}


