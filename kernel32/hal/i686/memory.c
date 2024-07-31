#include "memory.h"
#include "mm/palloc.h"
#include "hal/arch.h"
#include <stddef.h>
#include "common.h"
#include "mm/dynmap.h"
#include "ke/core/panic.h"
#include "ke/core/mutex.h"
#include "mm/mm.h"
#include "gdt.h"

#if defined(__i686__)

#define PAGE_FLAG_PRESENT 1
#define PAGE_FLAG_WRITABLE 2
#define PAGE_FLAG_USER 4
#define PAGE_FLAG_PWT 8
#define PAGE_FLAG_PCD 16

typedef uint32_t MmPageDirectoryEntry;
typedef uint32_t MmPageTableEntry;

//These are virtual addresses of current page directory and page tables
//They come from the self-referencing page directory trick
#define MM_PAGE_DIRECTORY_ADDRESS 0xFFFFF000
#define MM_FIRST_PAGE_TABLE_ADDRESS 0xFFC00000

MmPageDirectoryEntry *pageDir = (MmPageDirectoryEntry*)MM_PAGE_DIRECTORY_ADDRESS;
MmPageTableEntry *pageTable = (MmPageTableEntry*)MM_FIRST_PAGE_TABLE_ADDRESS;
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

STATUS HalGetPageFlags(uintptr_t vAddress, MmMemoryFlags *flags)
{
	*flags = 0;
	if(0 == (pageDir[vAddress >> 22] & PAGE_FLAG_PRESENT)) //check if page table is present
	{
		return MM_PAGE_NOT_PRESENT;
	}

	uint16_t f = PAGETABLE(vAddress >> 22, (vAddress >> 12) & 0x3FF) & 0xFFF;
	if(f & PAGE_FLAG_PRESENT)
		*flags |= MM_FLAG_PRESENT;
	if(f & PAGE_FLAG_WRITABLE)
		*flags |= MM_FLAG_WRITABLE;
	if(f & PAGE_FLAG_USER)
		*flags |= MM_FLAG_USER_MODE;
	if(f & PAGE_FLAG_PWT)
		*flags |= MM_FLAG_WRITE_THORUGH;
	if(f & PAGE_FLAG_PCD)
		*flags |= MM_FLAG_CACHE_DISABLE;
	return OK;
}

STATUS HalGetPhysicalAddress(uintptr_t vAddress, uintptr_t *pAddress)
{
	if(0 == (pageDir[vAddress >> 22] & PAGE_FLAG_PRESENT)) //check if page table is present
		return MM_PAGE_NOT_PRESENT;

	if(0 == (PAGETABLE(vAddress >> 22, (vAddress >> 12) & 0x3FF) & PAGE_FLAG_PRESENT)) //check if page is present
		return MM_PAGE_NOT_PRESENT;

	*pAddress = (PAGETABLE(vAddress >> 22, (vAddress >> 12) & 0x3FF) & 0xFFFFF000) + (vAddress & 0xFFF);
	return OK;
}

static KeSpinlock pageTableCreationMutex = KeSpinlockInitializer;

STATUS HalMapMemory(uintptr_t vAddress, uintptr_t pAddress, MmMemoryFlags flags)
{
	//make sure that no other function will try to create a page table for the same page directory entry
	KeAcquireSpinlock(&pageTableCreationMutex); 
	if((pageDir[vAddress >> 22] & PAGE_FLAG_PRESENT) == 0) //check if page table is present
	{
		//if not, create one
		uintptr_t pageTableAddr;
		if(MM_PAGE_TABLE_SIZE != MmAllocatePhysicalMemory(MM_PAGE_TABLE_SIZE, &pageTableAddr))
		{
			MmFreePhysicalMemory(pageTableAddr, MM_PAGE_TABLE_SIZE);
			KeReleaseSpinlock(&pageTableCreationMutex);
			return MM_NO_MEMORY;
		}

		pageDir[vAddress >> 22] = pageTableAddr | PAGE_FLAG_WRITABLE | PAGE_FLAG_PRESENT; //store page directory entry
		asm volatile("invlpg [%0]" : : "r" ((uintptr_t)(&PAGETABLE(vAddress >> 22, 0))) : "memory"); //invalidate old entry in TLB
		//now the table can be accessed

		for(uint16_t i = 0; i < MM_PAGE_TABLE_ENTRY_COUNT; i++)
		{
			PAGETABLE(vAddress >> 22, i) = 0; //clear page table
		}

		//from now on the page table can be accessed using pageTable[]
	}
	KeReleaseSpinlock(&pageTableCreationMutex);

	if(PAGETABLE(vAddress >> 22, (vAddress >> 12) & 0x3FF) & PAGE_FLAG_PRESENT) //check if page is already present
		return MM_ALREADY_MAPPED;

	uint16_t f = 0;
	if(flags & MM_FLAG_WRITABLE)
		f |= PAGE_FLAG_WRITABLE;
	if(flags & MM_FLAG_USER_MODE)
		f |= PAGE_FLAG_USER;
	if(flags & MM_FLAG_WRITE_THORUGH)
		f |= PAGE_FLAG_PWT;
	if(flags & MM_FLAG_CACHE_DISABLE)
		f |= PAGE_FLAG_PCD;
	PAGETABLE(vAddress >> 22, (vAddress >> 12) & 0x3FF) = (pAddress & 0xFFFFF000) | f | PAGE_FLAG_PRESENT; //add page to page table

	asm volatile("invlpg [%0]" : : "r" (vAddress) : "memory"); //invalidate old entry in TLB

	return OK;	
}

STATUS HalMapMemoryEx(uintptr_t vAddress, uintptr_t pAddress, uintptr_t size, MmMemoryFlags flags)
{
	STATUS ret = OK;
	size = ALIGN_UP(size, MM_PAGE_SIZE);
	while(size)
	{
		if(OK != (ret = HalMapMemory(vAddress, pAddress, flags)))
			return ret;
		
		vAddress += MM_PAGE_SIZE;
		pAddress += MM_PAGE_SIZE;

		size -= MM_PAGE_SIZE;
	}
	
	return OK;
}

STATUS HalUnmapMemory(uintptr_t vAddress)
{
	if((pageDir[vAddress >> 22] & PAGE_FLAG_PRESENT) == 0) //page table not present?
	{
		//this is not mapped
		KePanicEx(MEMORY_ACCESS_VIOLATION, MM_ALREADY_UNMAPPED, vAddress, 0, 0);
	}

	if(0 == (PAGETABLE(vAddress >> 22, (vAddress >> 12) & 0x3FF) & PAGE_FLAG_PRESENT))
	{
		//this memory is already unmapped
		KePanicEx(MEMORY_ACCESS_VIOLATION, MM_ALREADY_UNMAPPED, vAddress, 0, 0);
	}

	PAGETABLE(vAddress >> 22, (vAddress >> 12) & 0x3FF) = (MmPageTableEntry)0; //clear entry

	asm volatile("invlpg [%0]" : : "r" (vAddress) : "memory"); //invalidate old entry in TLB

	return OK;	
}

STATUS HalUnmapMemoryEx(uintptr_t vAddress, uintptr_t size)
{
	size = ALIGN_UP(size, MM_PAGE_SIZE);
	while(size)
	{
		HalUnmapMemory(vAddress);
		
		vAddress += MM_PAGE_SIZE;

		size -= MM_PAGE_SIZE;
	}
	return OK;
}

void HalInitVirtualAllocator(void)
{
	GdtInit();
	//iterate through the kernel space mapping
	for(uintptr_t i = (MM_KERNEL_ADDRESS >> 22); i < (MM_MEMORY_SIZE >> 22); i++)
	{
		/*
		The kernel space mapping must be kept consistent accross all tasks.
		Create all page tables for kernel and insert appropriate page directory entries.
		This way there will be only one set of kernel page tables for all tasks.
		*/

		if(pageDir[i] & PAGE_FLAG_PRESENT) //page table is already present, skip
			continue;
		
		uintptr_t newPageTable = allocatePageTable(); //allocate new page table
		
		pageDir[i] = newPageTable | PAGE_FLAG_WRITABLE | PAGE_FLAG_PRESENT; //add page directory entry

		asm volatile("invlpg [%0]" : : "r" ((uintptr_t)(&PAGETABLE(i, 0))) : "memory"); //invalidate old entry in TLB

		//clear page table
		CmMemset(&PAGETABLE(i, 0), 0, MM_PAGE_TABLE_ENTRY_COUNT * sizeof(MmPageTableEntry));
	}

	//clear all entries below kernel space
	CmMemset(pageDir, 0, (MM_KERNEL_ADDRESS >> 22) * sizeof(MmPageDirectoryEntry));
}

uintptr_t GetPageDirectoryAddress(void)
{
	uintptr_t pageDir;
	ASM("mov eax,cr3" : "=a" (pageDir) : : );
	return pageDir;
}

void SwitchPageDirectory(uintptr_t pageDir)
{
	uintptr_t oldPageDir;
	asm volatile("mov eax,cr3" : "=a" (oldPageDir) : : );
	if(oldPageDir != pageDir) //avoid TLB flush when both addresses are the same
		asm volatile("mov cr3,eax" : : "a" (pageDir) : );
}

uintptr_t CreateProcessPageDirectory(void)
{
	uintptr_t newPageDirAddress = allocatePageDirectory();
	if(0 == newPageDirAddress)
		return 0;
	
	MmPageDirectoryEntry *newPageDir = MmMapDynamicMemory(newPageDirAddress, MM_PAGE_DIRECTORY_SIZE, 0);

	if(NULL == newPageDir)
	{
		MmFreePhysicalMemory(newPageDirAddress, MM_PAGE_DIRECTORY_SIZE);
		return 0;
	}

	CmMemset(newPageDir, 0, MM_PAGE_DIRECTORY_SIZE);

	//copy entries for kernel space
	CmMemcpy(&newPageDir[MM_KERNEL_SPACE_START >> 22], 
		&pageDir[MM_KERNEL_SPACE_START >> 22], 
		((MM_KERNEL_SPACE_SIZE >> 22) - 1) * sizeof(MmPageDirectoryEntry));
	
	//apply self-referencing page directory trick
	newPageDir[MM_PAGE_DIRECTORY_ENTRY_COUNT - 1] = newPageDirAddress | PAGE_FLAG_WRITABLE | PAGE_FLAG_PRESENT;

	MmUnmapDynamicMemory(newPageDir);

	return newPageDirAddress;
}

#endif