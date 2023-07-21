#include "valloc.h"
#include "palloc.h"
#include <stddef.h>
#include "common.h"
#include "dynmap.h"
#include "ke/panic.h"
#include "ke/mutex.h"

// These are virtual addresses of current page directory and page tables
// They come from the self-referencing page directory trick
#define MM_PAGE_DIRECTORY_ADDRESS 0xFFFFF000
#define MM_FIRST_PAGE_TABLE_ADDRESS 0xFFC00000

MmPageDirectoryEntry_t *pageDir = (MmPageDirectoryEntry_t*)MM_PAGE_DIRECTORY_ADDRESS;
MmPageTableEntry_t *pageTable = (MmPageTableEntry_t*)MM_FIRST_PAGE_TABLE_ADDRESS;
#define PAGETABLE(table, entry) pageTable[MM_PAGE_TABLE_ENTRY_COUNT * (table) + (entry)]

#define MM_PAGE_DIRECTORY_SIZE 4096 //page directory size
#define MM_PAGE_TABLE_SIZE 4096 //page table size
#define MM_PAGE_DIRECTORY_ENTRY_COUNT 1024 //number of entries in page directory
#define MM_PAGE_TABLE_ENTRY_COUNT 1024 //number of entries in page table

/**
 * @brief Allocate memory for page directory
 * @return Allocated physical address or 0 on failure
*/
static uintptr_t allocatePageDirectory(void)
{
	uintptr_t address = 0;
	uintptr_t n = MmAllocatePhysicalMemory(MM_PAGE_DIRECTORY_SIZE, &address);
    if(MM_PAGE_DIRECTORY_SIZE != n)
    {
        MmFreePhysicalMemory(address, n);
        return 0;
    }
	return address;
}

/**
 * @brief Allocate memory for page tab;e
 * @return Allocated physical address or 0 on failure
*/
static uintptr_t allocatePageTable(void)
{
	uintptr_t address = 0;
	uintptr_t n = MmAllocatePhysicalMemory(MM_PAGE_TABLE_SIZE, &address);
    if(MM_PAGE_TABLE_SIZE != n)
    {
        MmFreePhysicalMemory(address, n);
        return 0;
    }
	return address;
}


STATUS MmGetPhysicalPageAddress(uintptr_t vAddress, uintptr_t *pAddress)
{
	if(0 == (pageDir[vAddress >> 22] & MM_PAGE_FLAG_PRESENT)) //check if page table is present
		return MM_PAGE_NOT_PRESENT;

	if(0 == (PAGETABLE(vAddress >> 22, (vAddress >> 12) & 0x3FF) & MM_PAGE_FLAG_PRESENT)) //check if page is present
		return MM_PAGE_NOT_PRESENT;

	*pAddress = (PAGETABLE(vAddress >> 22, (vAddress >> 12) & 0x3FF) & 0xFFFFF000) + (vAddress & 0xFFF);
	return OK;
}

static KeSpinLock_t pageTableCreationMutex;

STATUS MmMapMemory(uintptr_t vAddress, uintptr_t pAddress, MmPagingFlags_t flags)
{
	//make sure that no other function will try to create a page table for the same page directory entry
	KeAcquireSpinlockDisableIRQ(&pageTableCreationMutex); 
	if((pageDir[vAddress >> 22] & MM_PAGE_FLAG_PRESENT) == 0) //check if page table is present
	{
		//if not, create one
		uintptr_t pageTableAddr;
		if(MM_PAGE_TABLE_SIZE != MmAllocatePhysicalMemory(MM_PAGE_TABLE_SIZE, &pageTableAddr))
		{
			MmFreePhysicalMemory(pageTableAddr, MM_PAGE_TABLE_SIZE);
			return MM_NO_MEMORY;
		}

		pageDir[vAddress >> 22] = pageTableAddr | MM_PAGE_FLAG_WRITABLE | MM_PAGE_FLAG_PRESENT; //store page directory entry
		asm volatile("invlpg [%0]" : : "r" ((uintptr_t)(&PAGETABLE(vAddress >> 22, 0))) : "memory"); //invalidate old entry in TLB
		//now the table can be accessed

		for(uint16_t i = 0; i < MM_PAGE_TABLE_ENTRY_COUNT; i++)
		{
			PAGETABLE(vAddress >> 22, i) = 0; //clear page table
		}

		//from now on the page table can be accessed using pageTable[]
	}
	KeReleaseSpinlockEnableIRQ(&pageTableCreationMutex);

	if(PAGETABLE(vAddress >> 22, (vAddress >> 12) & 0x3FF) & MM_PAGE_FLAG_PRESENT) //check if page is already present
		return MM_ALREADY_MAPPED;

	PAGETABLE(vAddress >> 22, (vAddress >> 12) & 0x3FF) = (pAddress & 0xFFFFF000) | flags | MM_PAGE_FLAG_PRESENT; //add page to page table

	asm volatile("invlpg [%0]" : : "r" (vAddress) : "memory"); //invalidate old entry in TLB

	return OK;	
}

STATUS MmMapMemoryEx(uintptr_t vAddress, uintptr_t pAddress, uintptr_t size, MmPagingFlags_t flags)
{
	STATUS ret = OK;
	while(size)
	{
		if(OK != (ret = MmMapMemory(vAddress, pAddress, flags)))
			return ret;
		
		vAddress += MM_PAGE_SIZE;
		pAddress += MM_PAGE_SIZE;

		if(MM_PAGE_SIZE < size)
			size -= MM_PAGE_SIZE;
		else
			size = 0;
	}
	
	return OK;
}

STATUS MmUnmapMemory(uintptr_t vAddress)
{
	if((pageDir[vAddress >> 22] & MM_PAGE_FLAG_PRESENT) == 0) //page table not present?
	{
		//this is not mapped
		KePanicEx(PAGE_FAULT, MM_ALREADY_UNMAPPED, vAddress, 0, 0);
	}

	if(0 == (PAGETABLE(vAddress >> 22, (vAddress >> 12) & 0x3FF) & MM_PAGE_FLAG_PRESENT))
	{
		//this memory is already unmapped
		KePanicEx(PAGE_FAULT, MM_ALREADY_UNMAPPED, vAddress, 0, 0);
	}

	PAGETABLE(vAddress >> 22, (vAddress >> 12) & 0x3FF) = (MmPageTableEntry_t)0; //clear entry

	asm volatile("invlpg [%0]" : : "r" (vAddress) : "memory"); //invalidate old entry in TLB

	return OK;	
}

STATUS MmUnmapMemoryEx(uintptr_t vAddress, uintptr_t size)
{
	STATUS ret = OK;
	while(size)
	{
		MmUnmapMemory(vAddress);
		
		vAddress += MM_PAGE_SIZE;

		if(size > MM_PAGE_SIZE)
			size -= MM_PAGE_SIZE;
		else
			size = 0;
	}
	return OK;
}

void MmInitVirtualAllocator(void)
{
	//iterate through the kernel space mapping
	for(uintptr_t i = (MM_KERNEL_ADDRESS >> 22); i < (MM_MEMORY_SIZE >> 22); i++)
	{
		/*
		The kernel space mapping must be kept consistent accross all tasks.
		Create all page tables for kernel and insert appropriate page directory entries.
		This way there will be only one set of kernel page tables for all tasks.
		*/

		if(pageDir[i] & MM_PAGE_FLAG_PRESENT) //page table is already present, skip
			continue;
		
		uintptr_t newPageTable = allocatePageTable(); //allocate new page table
		
		pageDir[i] = newPageTable | MM_PAGE_FLAG_WRITABLE | MM_PAGE_FLAG_PRESENT; //add page directory entry

		asm volatile("invlpg [%0]" : : "r" ((uintptr_t)(&PAGETABLE(i, 0))) : "memory"); //invalidate old entry in TLB

		//clear page table
		CmMemset(&PAGETABLE(i, 0), 0, MM_PAGE_TABLE_ENTRY_COUNT * sizeof(MmPageTableEntry_t));
	}

	//clear all entries below kernel space
	CmMemset(pageDir, 0, (MM_KERNEL_ADDRESS >> 22) * sizeof(MmPageDirectoryEntry_t));
}

uintptr_t MmGetPageDirectoryAddress(void)
{
	uintptr_t pageDir;
	ASM("mov eax,cr3" : "=a" (pageDir) : );
	return pageDir;
}

void MmSwitchPageDirectory(uintptr_t pageDir)
{
	uintptr_t oldPageDir;
	asm volatile("mov eax,cr3" : "=a" (oldPageDir) : );
	if(oldPageDir != pageDir) //avoid TLB flush when the address is the same
		asm volatile("mov cr3,eax" : : "a" (pageDir) :);
}

uintptr_t MmCreateProcessPageDirectory(void)
{
	uintptr_t newPageDirAddress = allocatePageDirectory();
	if(0 == newPageDirAddress)
		return 0;
	
	MmPageDirectoryEntry_t *newPageDir = MmMapDynamicMemory(newPageDirAddress, MM_PAGE_DIRECTORY_SIZE, 0);

	if(NULL == newPageDir)
	{
		MmFreePhysicalMemory(newPageDirAddress, MM_PAGE_DIRECTORY_SIZE);
		return 0;
	}

	CmMemset(newPageDir, 0, MM_PAGE_DIRECTORY_SIZE);

	//copy entries for kernel space
	CmMemcpy(&newPageDir[MM_KERNEL_SPACE_START >> 22], 
		&pageDir[MM_KERNEL_SPACE_START >> 22], 
		((MM_KERNEL_SPACE_SIZE >> 22) - 1) * sizeof(MmPageDirectoryEntry_t));
	
	//apply self-referencing page directory trick
	newPageDir[MM_PAGE_DIRECTORY_ENTRY_COUNT - 1] = newPageDirAddress | MM_PAGE_FLAG_WRITABLE | MM_PAGE_FLAG_PRESENT;

	MmUnmapDynamicMemory(newPageDir, MM_PAGE_DIRECTORY_SIZE);

	return newPageDirAddress;
}