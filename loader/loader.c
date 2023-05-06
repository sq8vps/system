#include "loader.h"
#include "disp.h"
#include "pci.h"
#include "ata.h"
#include "defines.h"
#include "fat.h"
#include "mm.h"
#include "elf.h"

static uint64_t prepareKernelMemory(uint32_t kernelSize);
static void loader_findAddress(uint64_t bytes, uint64_t *addr, uint64_t *size);

uint8_t loader_bootDrive = 255; //diskTable boot disk entry number

#define MBR_SIGNATURE_OFFSET 440

uint8_t loader_buf[512] = {0}; //sector buffer

extern uint32_t pageUsageTable[];

struct KerMem
{
	uint64_t *pageDir;
	uint64_t *firstPageTable;
	uint16_t pageTableCount;
	uint64_t *kernelAddr;
};

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

 __attribute__ ((section (".ldr"))) void loaderEntry(void)
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
		printf("\nBoot failed: system drive not found\n");
#endif
		loop();
	}

	Fat_init(&(diskTable[loader_bootDrive]), 0);
	Fat_changeDir(&fatDisk, "/system/");

	uint32_t kernelEntry = 0;
	if(Elf_load("KERNEL32.ELF", &kernelEntry) == OK)
	{
		printf("Kernel loaded successfully. Entry point at %X\n", kernelEntry);
		while(1);;
	}


	asm volatile("mov eax, %0" : : "d" (pageUsageTable) : );
	asm volatile("jmp %0" : : "d" (kernelEntry) : );




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





