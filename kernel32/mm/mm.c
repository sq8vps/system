#include "mm.h"

#define MM_PAGE_DIRECTORY_ADDRESS 0xFFFFF000
#define MM_FIRST_PAGE_TABLE_ADDRESS 0xFFC00000

/**
 * @brief Page frame usage lookup bitmap table
 *
 * Contains one bit for every page frame. 0 - page frame is free to use, 1 - page frame is in use or unavailable
 */
uint32_t pageUsageTable[(2 << 27) / MM_PAGE_SIZE] = {0};

#define MARK_PAGE_USED(n) (pageUsageTable[(n) >> 5] |= (1 << ((n) & 31)))
#define MARK_PAGE_FREE(n) (pageUsageTable[(n) >> 5] &= ~(1 << ((n) & 31)))

#define PAGE_IDX_TO_PHYS(n) (uint32_t)((n) * MM_PAGE_SIZE)

uint32_t kernelPageDirAddr = 0;

static uint32_t mm_findFreeFrame(void);

kError_t Mm_init(uint32_t usageTable, uint32_t pageDir)
{
	for(uint32_t i = 0; i < (sizeof(pageUsageTable) / sizeof(*pageUsageTable)); i++) //copy page usage table
		pageUsageTable[i] = ((uint32_t*)usageTable)[i];

	kernelPageDirAddr = pageDir;

	return OK;
}

kError_t Mm_createPageDir(uint32_t *pageDir)
{
	uint32_t pageDirIdx = mm_findFreeFrame(); //find frame for page directory
	if(pageDirIdx == 0)
		return MM_NO_MEMORY;

	MARK_PAGE_USED(pageDirIdx); //mark frame as used

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
kError_t Mm_allocateContiguous(uint32_t pages, uint32_t vAddress, uint8_t flags, uint8_t align)
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
								kError_t ret = Mm_mapPage(vAddress + k * MM_PAGE_SIZE, PAGE_IDX_TO_PHYS(firstPage + k), flags);
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

/**
 * @brief Get physical address corresponding to the virtual address
 * @param vAddress Input virtual address
 * @param *pAddress Output physical address
 */
kError_t Mm_getPhysAddr(uint32_t vAddress, uint32_t *pAddress)
{
	uint32_t *pageDir = (uint32_t*)MM_PAGE_DIRECTORY_ADDRESS; //get page directory

	if(pageDir[vAddress >> 22] & 1 == 0) //check if page table is present
		return MM_PAGE_NOT_PRESENT;

	uint32_t *pageTable = (uint32_t*)MM_FIRST_PAGE_TABLE_ADDRESS + ((vAddress & 0xFFC00000) >> 12); //calculate appropriate page table address

	if(pageTable[(vAddress >> 12) & 0x3FF] & MM_PAGE_FLAG_PRESENT == 0) //check if page is present
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
kError_t Mm_mapPage(uint32_t vAddress, uint32_t pAddress, uint8_t flags)
{
	uint32_t *pageDir = (uint32_t*)MM_PAGE_DIRECTORY_ADDRESS; //get page directory
	uint32_t *pageTable = (uint32_t*)(MM_FIRST_PAGE_TABLE_ADDRESS + ((vAddress & 0xFFC00000) >> 10)); //calculate appropriate page table address

	if((pageDir[vAddress >> 22] & 1) == 0) //check if page table is present
	{
		//if not, create one
		uint32_t frame = mm_findFreeFrame();
		if(frame == 0)
			return MM_NO_MEMORY;

		MARK_PAGE_USED(frame);

		pageDir[vAddress >> 22] = PAGE_IDX_TO_PHYS(frame) | MM_PAGE_FLAG_USER | MM_PAGE_FLAG_WRITABLE | MM_PAGE_FLAG_PRESENT; //store page directory entry
		asm volatile("invlpg [%0]" : : "r" (pageTable) : "memory"); //invalidate old entry in TLB
		//now, the table can be accessed

		for(uint16_t i = 0; i < 1024; i++)
		{
			pageTable[i] = 0; //clear page table
		}
	}

	if(pageTable[(vAddress >> 12) & 0x3FF] & MM_PAGE_FLAG_PRESENT) //check if page is already present
		return MM_ALREADY_MAPPED;

	pageTable[(vAddress >> 12) & 0x3FF] = (pAddress & 0xFFFFF000) | flags | MM_PAGE_FLAG_PRESENT;


	asm volatile("invlpg [%0]" : : "r" (vAddress) : "memory"); //invalidate old entry in TLB

	return OK;
}

/**
 * @brief Allocate memory and map it to virtual address
 * @param vAddress Virtual address
 * @param flags Flags for the mapped page
 */
kError_t Mm_allocate(uint32_t vAddress, uint8_t flags)
{
	uint32_t fr = mm_findFreeFrame(); //look for free frame page
	if(fr == 0)
		return MM_NO_MEMORY;

	MARK_PAGE_USED(fr);
	return Mm_mapPage(vAddress, PAGE_IDX_TO_PHYS(fr), flags);
}

kError_t Mm_allocateEx(uint32_t vAddress, uint32_t pages, uint8_t flags)
{
	kError_t ret = OK;
	for(uint32_t i = 0; i < pages; i++)
	{
		ret = Mm_allocate(vAddress + i * MM_PAGE_SIZE, flags);
		if(ret != OK)
			break;
	}
	return ret;
}
