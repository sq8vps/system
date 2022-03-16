#include "loader.h"
#include "disp.h"
#include "pci.h"
#include "ata.h"
#include "defines.h"
#include "fat.h"
#include "paging.h"

static uint64_t prepareKernelMemory(uint32_t kernelSize);
static void loader_findAddress(uint64_t bytes, uint64_t *addr, uint64_t *size);

uint8_t loader_bootDrive = 255; //diskTable boot disk entry number

#define MBR_SIGNATURE_OFFSET 440

uint8_t loader_buf[4096] = {0}; //sector buffer

uint8_t test_buf[32768] __attribute__ ((aligned(8)));

struct KerMem
{
	uint64_t *pageDir;
	uint64_t *firstPageTable;
	uint16_t pageTableCount;
	uint64_t *kernelAddr;
};

/**
 * This is the 32-bit EAEOS bootloader.
 *
 *
 *
 *
 *
 */

static void loop(void)
{
	asm("ldrLoop: cli");
	asm("hlt");
	asm("jmp ldrLoop");
}



/**
 * \brief Bootloader (3rd stage) entry point
 * \attention This function MUST be kept at the strict beginning of the binary file
 * \warning This function never returns
 */

 __attribute__ ((section (".ldr"))) volatile void loaderEntry(void)
{
	disp_clear(); //clear screen
	ata_init(); //init ATA
	disk_init();
	ata_setAddDiskCallback(&disk_add);
	pci_setAtaEnumCallback(&ata_registerController); //set callback function for ATA controller enumeration
	pci_scanAll(); //scan PCI buses
	ata_enableControllers(); //set up ATA controllers

	//look for the boot drive number
	for(uint8_t i = 0; i < DISK_TABLE_MAX_ENTRIES; i++)
	{
		if(diskTable[i].present == 1)
		{
			if(disk_read(diskTable[i], loader_buf, 0, 1) == DISK_OK) //read 0th sector to determine disk signature
			{

				if((*((uint32_t*)(loader_buf + MBR_SIGNATURE_OFFSET)) == *((uint32_t*)DISK_SIG))) //check if the signature matches the signature stored by the 2nd stage bootloader
				{
					loader_bootDrive = i; //if so, store drive number
				}
			}
		}
		else break; //if this disk was not present, next also can't be
	}

	if(loader_bootDrive == 255) //if boot drive was not found (this shouldn't happen if we are already in the bootloader, but whatever)
	{
#if __DEBUG > 0
		printf("\nBoot failed: system drive not found\n");
#endif
		loop();
	}

	Fat_init(&(diskTable[loader_bootDrive]), 0);
	Fat_changeDir(&fatDisk, "/system/");



	printf("Returned %d\n", (int)Fat_readFile(&fatDisk, "LDR32", 0, 4096, test_buf));

	Paging_construct(0x01000000, KERNEL_VIRTUAL_ADDRESS);
	Paging_enable();

	printf("Paging enabled\n");


	loop();
}


/*static uint64_t prepareKernelMemory(uint32_t kernelSize)
{
	struct KerMem mem;
	mem.pageTableCount = 1 + MIN_KERNEL_MEMORY / 4096;

	uint64_t *dummy;
	//MIN_KERNEL_MEMORY stores required memory for kernel itself. Here we also need additional memory for one page directory and ceil(MIN_KERNEL_MEMORY/4 MiB) page tables (each page table can address 4 MiB of physical memory)
	loader_findAddress(MIN_KERNEL_MEMORY + PAGE_SIZE * (1 + mem.pageTableCount), mem.pageDir, dummy); //find memory region for kernel

	if(*mem.kernelAddr == 0)
	{
#if __DEBUG >0
		printf("\nBoot failed: not enough memory to load kernel\n");
#endif
		loop();
	}
	else
	{

	}
 2


}*/


/**
 * \brief Find the lowest memory region in the extended memory of, at least, a specified size, aligned to PAGING_PAGE_SIZE
 * \param bytes Required size in bytes
 * \param *addr The base address if found, otherwise 0
 * \param *size Size if region found, otherwise 0
 * \attention It uses the memory map provided by the 2nd stage bootloader, starting from physical address MEMORY_MAP
 */
static void loader_findAddress(uint64_t bytes, uint64_t *addr, uint64_t *size)
{
	*addr = 0;
	*size = 0;
	uint16_t numOfEntries = *((uint16_t*)MEMORY_MAP);  //the first word contains number of entries
	if(numOfEntries > MEMORY_MAP_MAX_ENTRY_COUNT)
		numOfEntries = MEMORY_MAP_MAX_ENTRY_COUNT;
	for(uint16_t i = 0; i < numOfEntries; i++)
	{
		uint64_t base = *((uint64_t*)(MEMORY_MAP + 2 + i * MEMORY_MAP_ENTRY_SIZE)); //next 8 bytes are the base address
		if(base < EXTENDED_MEMORY_START)
			continue; //we are interested only in extended memory
		uint64_t len = *((uint64_t*)(MEMORY_MAP + 10 + i * MEMORY_MAP_ENTRY_SIZE)); //next 8 bytes are the size
		if(len < bytes)
			continue; //size is less than required
		uint64_t attr = *((uint64_t*)(MEMORY_MAP + 18 + i * MEMORY_MAP_ENTRY_SIZE));
		if((attr & 0x1FFFFFFFF) != 0x100000001)
			continue; //type field must be equal to 1 and and optional ACPI 3.0 bit must be set

		if(base & (PAGING_PAGE_SIZE - 1)) //if memory address is not aligned to page size
		{
			uint16_t shift = PAGING_PAGE_SIZE - (base & (PAGING_PAGE_SIZE - 1)); //calculate required shift to align
			base += shift;
			len -= shift;

			if(len < bytes) //check if size is still enough
				continue;

			if(len & (PAGING_PAGE_SIZE - 1)) //check if size is a multiplicity of the page size
			{
				//if not
				shift = PAGING_PAGE_SIZE - (len & (PAGING_PAGE_SIZE - 1));
				len -= shift;
			}

			if(len < bytes) //check if size is still enough
				continue;

			//otherwise it's ok
		}

		//if everything went OK
		*addr = base;
		*size = len;
		break;
	}
	return;
}


