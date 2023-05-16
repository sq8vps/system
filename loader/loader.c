#include "loader.h"
#include "disp.h"
#include "pci.h"
#include "ata.h"
#include "defines.h"
#include "fat.h"
#include "mm.h"
#include "elf.h"
#include "../cdefines.h"

uint8_t loader_bootDrive = 255; //diskTable boot disk entry number

#define MBR_SIGNATURE_OFFSET 440

uint8_t loader_buf[512] = {0}; //sector buffer

extern uint32_t pageUsageTable[];

enum FileType
{
	FILE_KERNEL = 0,
	FILE_DRIVER = 1,
	FILE_OTHER = 2,
};

#define FILE_LIST_ENTRY_NAME_SIZE 30

struct FileListEntry
{
	char name[FILE_LIST_ENTRY_NAME_SIZE];
	enum FileType type;
};

struct FileListEntry fileList[] = 
{
	{.name = "kernel32.elf", .type = FILE_KERNEL},
	{.name = "kernel32.elf", .type = FILE_OTHER},
	{.name = "vga.drv", .type = FILE_DRIVER},
};

/**
 * \brief Bootloader (3rd stage) entry point
 * \attention This function MUST be kept at the strict beginning of the binary file
 * \warning This function never returns
 */

 __attribute__ ((section (".ldr"))) 
 void loaderEntry(void)
{
	disp_clear(); //clear screen

	uint32_t pageDirAddr;
	Mm_init(&pageDirAddr);
	Mm_enablePaging(pageDirAddr);

	ata_init(); //init ATA
	Disk_init();
	ata_setAddDiskCallback(&Disk_add);
	pci_setAtaEnumCallback(&ata_registerController); //set callback function for ATA controller enumeration
	pci_scanAll(); //scan PCI buses
	ata_enableControllers(); //set up ATA controllers

	//look for the boot drive number
	for(uint8_t i = 0; i < DISK_TABLE_MAX_ENTRIES; i++)
	{
		if(diskTable[i].present == 1)
		{
			if(Disk_read(diskTable[i], loader_buf, 0, 0, 512) == OK) //read 0th sector to determine disk signature
			{
				if((*((uint32_t*)(loader_buf + MBR_SIGNATURE_OFFSET)) == *((uint32_t*)DISK_SIG))) //check if the signature matches the signature stored by the 2nd stage bootloader
				{
					loader_bootDrive = i; //if so, store drive number
				}
			}
		}
		else break; //if this disk was not present, the next one also can't be
	}

	if(loader_bootDrive == 255) //if boot drive was not found (this shouldn't happen if we are already in the bootloader, but whatever)
	{
#if __DEBUG > 0
		printf("Boot failed: system drive not found\n");
#endif
		while(1);;
	}

	Fat_init(&(diskTable[loader_bootDrive]), 0);
	

	uint32_t kernelEntry = 0;
	error_t ret = OK;

	for(uint16_t i = 0; i < (sizeof(fileList) / sizeof(*fileList)); i++)
	{
		struct FileListEntry *e = &(fileList[i]); 
#if __DEBUG > 0
		printf("Loading %s...\n", e->name);
#endif
		if(e->type == FILE_KERNEL)
		{
			if(OK != (ret = Fat_changeDir(&fatDisk, "/system/")))
				break;
			if(OK != (ret = Elf_loadExec(e->name, &kernelEntry)))
				break;
		}
		else if(e->type == FILE_DRIVER)
		{
			if(OK != (ret = Fat_changeDir(&fatDisk, "/system/drivers/")))
				break;
			if(OK != (ret = Elf_loadDriver(e->name)))
				break;
		}
		else if(e->type == FILE_OTHER)
		{
			if(OK != (ret = Fat_changeDir(&fatDisk, "/system/")))
				break;
			if(OK != (ret = Elf_loadOther(e->name)))
				break;
		}
		else
		{
#if __DEBUG > 0
			printf("Unknown file type %d\n", (int)e->type);
#endif
			ret = ELF_UNSUPPORTED_TYPE;
			break;
		}
	}

	if(OK != ret)
	{
#if __DEBUG > 0
		printf("Boot failed: at least one component cannot be loaded\n");
#endif
		while(1);;
	}

	//fill kernel entry parameters structure
	struct KernelEntryArgs kernelArgs;
	kernelArgs.kernelPageDir = pageDirAddr;
	kernelArgs.pageUsageTable = (uint32_t)&pageUsageTable;
	uintptr_t rawElfTableSize = 0;
	kernelArgs.rawElfTable = Elf_getRawELFTable(&rawElfTableSize);
	kernelArgs.rawElfTableSize = rawElfTableSize;

	//call kernel
	(*((void(*)(struct KernelEntryArgs))kernelEntry))(kernelArgs);

	while(1);;
}


