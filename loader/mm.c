#include "mm.h"
#include "defines.h"
#include "disp.h"


/**
 * @brief Page frame usage lookup bitmap table
 *
 * Contains one bit for every page frame. 0 - page frame is free to use, 1 - page frame is in use or unavailable
 */
uint32_t pageUsageTable[(2 << 27) / MM_PAGE_SIZE] = {0};

//#define MARK_PAGE_USED(base) (pageUsageTable[(base) / (MM_PAGE_SIZE * 32)] |= (1 << ((base) & 31)))
//#define MARK_PAGE_FREE(base) (pageUsageTable[(base) / (MM_PAGE_SIZE * 32)] &= ~(1 << ((base) & 31)))
#define MARK_PAGE_USED(n) (pageUsageTable[(n) >> 5] |= (1 << ((n) & 31)))
#define MARK_PAGE_FREE(n) (pageUsageTable[(n) >> 5] &= ~(1 << ((n) & 31)))

#define PAGE_IDX_TO_PHYS(n) (uint32_t)((n) * MM_PAGE_SIZE)

static uint32_t mm_findFreeFrame(void);

error_t Mm_init(uint32_t *pageDir)
{
	for(uint32_t i = 0; i < (2 << 27) / MM_PAGE_SIZE; i++) //first mark all pages as used/unavailable
		pageUsageTable[i] = 0xFFFFFFFF;

	//then look for usable pages
	for(uint16_t i = 0; i < *((uint16_t*)MM_BIOS_MEMORY_MAP); i++) //read BIOS memory map. The first word is the number of entries
	{
		uint64_t base = *((uint64_t*)(MM_BIOS_MEMORY_MAP + 2 + i * MM_BIOS_MEMORY_MAP_ENTRY_SIZE)); //next 8 bytes are the base address
		uint64_t len = *((uint64_t*)(MM_BIOS_MEMORY_MAP + 10 + i * MM_BIOS_MEMORY_MAP_ENTRY_SIZE)); //next 8 bytes are the size
		uint64_t attr = *((uint64_t*)(MM_BIOS_MEMORY_MAP + 18 + i * MM_BIOS_MEMORY_MAP_ENTRY_SIZE)); //and attributes
		if((attr & 0x1FFFFFFFF) != 0x100000001) //type field must be equal to 1 and and optional ACPI 3.0 bit must be set.
			continue; //if not, this memory region is unavailable
		if(base < 0x100000) //also skip lower memory (<1 MiB)
			continue;

		if(base & (MM_PAGE_SIZE - 1)) //if memory address is not aligned to page size
		{
			uint16_t shift = MM_PAGE_SIZE - (base & (MM_PAGE_SIZE - 1)); //calculate required shift to align
			base += shift;
			len -= shift;

			if(len < MM_PAGE_SIZE) //check if the region is at least 1 page in size
				continue; //if not, this regions is unusable

			if(len & (MM_PAGE_SIZE - 1)) //check if size is a multiple of the page size
			{
				//if not, align the size again
				shift = MM_PAGE_SIZE - (len & (MM_PAGE_SIZE - 1));
				len -= shift;
			}

			if(len < MM_PAGE_SIZE) //check if the region is at least 1 page in size
				continue; //if not, this regions is unusable

			//otherwise it's usable

		}

		for(uint32_t i = 0; i < (len / MM_PAGE_SIZE); i++) //loop for all page frames
			MARK_PAGE_FREE(base / MM_PAGE_SIZE + i); //mark page frame as free
	}

	uint32_t pageTableIdx = mm_findFreeFrame(); //find frame for identity paging page table
	if(pageTableIdx == 0)
		return MM_NO_MEMORY;

	MARK_PAGE_USED(pageTableIdx);

	for(uint16_t i = 0; i < 256; i++) //set up identity paging for the lowest 1 MiB
	{
		*((uint32_t*)(PAGE_IDX_TO_PHYS(pageTableIdx) + (i << 2))) = (MM_PAGE_SIZE * i) | 0x3; //set page frame address and flags: present, writable, supervisor only
	}

	for(uint16_t i = 256; i < 1024; i++) //clear remaining entries
	{
		*((uint32_t*)(PAGE_IDX_TO_PHYS(pageTableIdx) + (i << 2))) = 0;
	}

	if(Mm_newPageDir(pageDir) != OK)
		return MM_NO_MEMORY;

	*((uint32_t*)(*pageDir)) = PAGE_IDX_TO_PHYS(pageTableIdx) | 0x3; //write page table address and set flags: present, writable, supervisor only

	return OK;
}

error_t Mm_newPageDir(uint32_t *pageDir)
{
	uint32_t pageDirIdx = mm_findFreeFrame(); //find frame for page directory
	if(pageDirIdx == 0)
		return MM_NO_MEMORY;

	MARK_PAGE_USED(pageDirIdx);

	*pageDir = PAGE_IDX_TO_PHYS(pageDirIdx); //get page directory physical address

	for(uint16_t i = 0; i < 1024; i++) //clear it first
	{
		*((uint32_t*)(*pageDir + (i << 2))) = 0;
	}

	//self-referencing page directory trick
	//the page directory is the last page table at the same time
	//page directory contains entries that point to page tables
	//because of that page tables themselves are mapped to the highest 4 MiB of the virtual memory
	//and can be accessed using their virtual addresses
	//the page directory itself is mapped to the highest address (0xFFFFF000)
	*((uint32_t*)(*pageDir + (1023 << 2))) = *pageDir | 0x3;

	return OK;
}

/**
 * @brief Find first available page frame
 * @return Page frame index/number (not address!), 0 if no free memory
 */
static uint32_t mm_findFreeFrame(void)
{
	for(uint32_t i = 0; i < (2 << 27) / MM_PAGE_SIZE; i++)
	{
		if(pageUsageTable[i] != 0xFFFFFFFF) //if there is a free page (at least one bit is not set)
		{
			for(uint8_t j = 0; j < 32; j++)
			{
				if((pageUsageTable[i] & (1 << j)) == 0) //bit is not set
				{
					return (i << 5) + j; //return page frame index
				}
			}
		}
	}
	return 0;
}


/**
 * @brief Allocate and map a block of physically contiguous pages
 * @param pages Number of pages to allocate
 * @param vAddress Starting virtual address
 * @param flags Page flags
 * @param align Alignment value in pages (0 or 1 for 1 page alignment)
 */
error_t Mm_allocateContiguous(uint32_t pages, uint32_t vAddress, uint8_t flags, uint8_t align)
{
	if(pages == 0)
		return OK;

	uint32_t firstPage = 0;
	uint32_t size = 0;

	for(uint32_t i = 0; i < (2 << 27) / MM_PAGE_SIZE; i++)
	{
		firstPage = 0;
		if(pageUsageTable[i] != 0xFFFFFFFF) //if there is a free page (at least one bit is not set)
		{
			for(uint8_t j = 0; j < 32; j++)
			{
				if((pageUsageTable[i] & (1 << j)) == 0) //bit is not set
				{
					firstPage = (i << 5) + j; //store first possible page
					if((align > 0) && (firstPage % align)) //check if address is aligned properly
						continue;
					size = 0;
					while((pageUsageTable[i] & (1 << j)) == 0) //loop for consecutive free pages
					{
						size++;
						if(size == pages) //enough page frames were found
						{
							for(uint32_t k = 0; k < size; k++) //loop for all page frames
							{
								MARK_PAGE_USED(firstPage + k); //mark them as used
								error_t ret = Mm_mapPage(vAddress + k * MM_PAGE_SIZE, PAGE_IDX_TO_PHYS(firstPage + k), flags);
								if(ret != OK)
									return ret;
							}
							return OK;
						}

						j++; //increment bit index
						if(j == 32) //all 32 bits checked
						{
							j = 0; //reset bit index
							i++; //go to the next bitmap entry
						}

						if(i == (2 << 27) / MM_PAGE_SIZE) //end of bitmap reached, contiguous frames not found
							return MM_NO_MEMORY;
					}
				}
			}
		}
	}
	return MM_NO_MEMORY;
}


void Mm_enablePaging(uint32_t pageDir)
{
	asm("mov cr3,eax" : : "a" (pageDir)); //store page directory address in CR3
	asm("mov eax,cr0");
	asm("or eax,0x80000000"); //enable paging in CR0
	asm("mov cr0,eax");
}

/**
 * @brief Get physical address corresponding to the virtual address
 * @param vAddress Input virtual address
 * @param *pAddress Output physical address
 */
error_t Mm_getPhysAddr(uint32_t vAddress, uint32_t *pAddress)
{
	uint32_t *pageDir = (uint32_t*)0xFFFFF000; //page directory is in the last page of the virtual memory

	if(pageDir[vAddress >> 22] & 1 == 0) //check if page table is present
		return MM_PAGE_NOT_PRESENT;

	uint32_t *pageTable = (uint32_t*)0xFFC00000 + ((vAddress & 0xFFC00000) >> 12); //calculate appropriate page table address

	if(pageTable[(vAddress >> 12) & 0x3FF] & 1 == 0) //check if page is present
		return MM_PAGE_NOT_PRESENT;

	*pAddress = (pageTable[(vAddress >> 12) & 0x3FF] & 0xFFFFF000) + (vAddress & 0xFFF);
	return OK;
}

/**
 * @brief Map physical address to virtual address
 * @param vAddress Virtual address
 * @param pAddress Physical address
 * @param flags Flags for the mapped page
 */
error_t Mm_mapPage(uint32_t vAddress, uint32_t pAddress, uint8_t flags)
{
	uint32_t *pageDir = (uint32_t*)0xFFFFF000; //page directory is in the last page of the virtual memory
	uint32_t *pageTable = (uint32_t*)(0xFFC00000 + ((vAddress & 0xFFC00000) >> 10)); //calculate appropriate page table address

	if((pageDir[vAddress >> 22] & 1) == 0) //check if page table is present
	{
		//if not, create one
		uint32_t fr = mm_findFreeFrame();
		if(fr == 0)
			return MM_NO_MEMORY;

		MARK_PAGE_USED(fr);

		pageDir[vAddress >> 22] = PAGE_IDX_TO_PHYS(fr) | 0x7; //store page directory entry
		asm volatile("invlpg [%0]" : : "r" (pageTable) : "memory");
		//now, the table can be accessed

		for(uint16_t i = 0; i < 1024; i++)
		{
			pageTable[i] = 0; //clear page table
		}
	}

	if(pageTable[(vAddress >> 12) & 0x3FF] & 1) //check if page is already present
		return MM_PAGE_ALREADY_MAPPED;

	pageTable[(vAddress >> 12) & 0x3FF] = (pAddress & 0xFFFFF000) | flags | MM_PAGE_FLAG_PRESENT;


	asm volatile("invlpg [%0]" : : "r" (vAddress) : "memory");

	return OK;
}

/**
 * @brief Allocate memory and map it to virtual address
 * @param vAddress Virtual address
 * @param flags Flags for the mapped page
 */
error_t Mm_allocate(uint32_t vAddress, uint8_t flags)
{
	uint32_t fr = mm_findFreeFrame(); //look for free frame page
	if(fr == 0)
		return MM_NO_MEMORY;

	MARK_PAGE_USED(fr);
	return Mm_mapPage(vAddress, PAGE_IDX_TO_PHYS(fr), flags);
}

error_t Mm_allocateEx(uint32_t vAddress, uint32_t pages, uint8_t flags)
{
	error_t ret = OK;
	for(uint32_t i = 0; i < pages; i++)
	{
		ret = Mm_allocate(vAddress + i * MM_PAGE_SIZE, flags);
		if(ret != OK)
			break;
	}
	return ret;
}
