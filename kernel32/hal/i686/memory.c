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
#include "ke/sched/sched.h"
#include "ipi.h"

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

volatile MmPageDirectoryEntry *const pageDir = (MmPageDirectoryEntry*)MM_PAGE_DIRECTORY_ADDRESS;
volatile MmPageTableEntry *const pageTable = (MmPageTableEntry*)MM_FIRST_PAGE_TABLE_ADDRESS;
#define PAGETABLE(table, entry) pageTable[MM_PAGE_TABLE_ENTRY_COUNT * (table) + (entry)]

#define MM_PAGE_DIRECTORY_SIZE 4096 //page directory size
#define MM_PAGE_TABLE_SIZE 4096 //page table size
#define MM_PAGE_DIRECTORY_ENTRY_COUNT 1024 //number of entries in page directory
#define MM_PAGE_TABLE_ENTRY_COUNT 1024 //number of entries in page table

#define IS_KERNEL_MEMORY(address) ((((address) >= MM_KERNEL_SPACE_START) && ((address) < MM_FIRST_PAGE_TABLE_ADDRESS)) \
		|| ((address) >= (uintptr_t)&pageTable[MM_KERNEL_SPACE_START >> 12]))

static KeSpinlock I686KernelMemoryLock = KeSpinlockInitializer;

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

static inline PRIO I686AcquireMemoryLock(uintptr_t address)
{
#ifndef SMP
	return HalRaisePriorityLevel(HAL_PRIORITY_LEVEL_EXCLUSIVE);
#else
	if(IS_KERNEL_MEMORY(address) || (NULL == KeGetCurrentTask()))
	{
		return KeAcquireSpinlock(&I686KernelMemoryLock);
	}
	else
	{
		return KeAcquireSpinlock(KeGetCurrentTask()->cpu.userMemoryLock);
	}
#endif
}

static inline void I686ReleaseMemoryLock(uintptr_t address, PRIO lastPriority)
{
#ifndef SMP
	HalLowerPriorityLevel(lastPriority);
#else
	if(IS_KERNEL_MEMORY(address) || (NULL == KeGetCurrentTask()))
	{
		KeReleaseSpinlock(&I686KernelMemoryLock, lastPriority);
	}
	else
	{
		KeReleaseSpinlock(KeGetCurrentTask()->cpu.userMemoryLock, lastPriority);
	}
#endif
}

STATUS HalGetPageFlags(uintptr_t vAddress, MmMemoryFlags *flags)
{
	*flags = 0;
	PRIO prio = I686AcquireMemoryLock(vAddress);
	if(0 == (pageDir[vAddress >> 22] & PAGE_FLAG_PRESENT)) //check if page table is present
	{
		I686ReleaseMemoryLock(vAddress, prio);
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
		*flags |= MM_FLAG_WRITE_THROUGH;
	if(f & PAGE_FLAG_PCD)
		*flags |= MM_FLAG_CACHE_DISABLE;
	I686ReleaseMemoryLock(vAddress, prio);
	return OK;
}

STATUS HalGetPhysicalAddress(uintptr_t vAddress, uintptr_t *pAddress)
{
	PRIO prio = I686AcquireMemoryLock(vAddress);
	if(0 == (pageDir[vAddress >> 22] & PAGE_FLAG_PRESENT)) //check if page table is present
	{
		I686ReleaseMemoryLock(vAddress, prio);
		return MM_PAGE_NOT_PRESENT;
	}

	if(0 == (PAGETABLE(vAddress >> 22, (vAddress >> 12) & 0x3FF) & PAGE_FLAG_PRESENT)) //check if page is present
	{
		I686ReleaseMemoryLock(vAddress, prio);
		return MM_PAGE_NOT_PRESENT;
	}

	*pAddress = (PAGETABLE(vAddress >> 22, (vAddress >> 12) & 0x3FF) & 0xFFFFF000) + (vAddress & 0xFFF);
	I686ReleaseMemoryLock(vAddress, prio);
	return OK;
}

MmMemoryFlags I686GetPageFlagsFromPageFault(uintptr_t address)
{
	MmMemoryFlags flags = 0;
	if(0 == (pageDir[address >> 22] & PAGE_FLAG_PRESENT)) //check if page table is present
	{
		return 0;
	}

	uint16_t f = PAGETABLE(address >> 22, (address >> 12) & 0x3FF) & 0xFFF;
	if(f & PAGE_FLAG_PRESENT)
		flags |= MM_FLAG_PRESENT;
	if(f & PAGE_FLAG_WRITABLE)
		flags |= MM_FLAG_WRITABLE;
	if(f & PAGE_FLAG_USER)
		flags |= MM_FLAG_USER_MODE;
	if(f & PAGE_FLAG_PWT)
		flags |= MM_FLAG_WRITE_THROUGH;
	if(f & PAGE_FLAG_PCD)
		flags |= MM_FLAG_CACHE_DISABLE;
	return OK;	
}

STATUS HalMapMemory(uintptr_t vAddress, uintptr_t pAddress, MmMemoryFlags flags)
{
	PRIO prio = I686AcquireMemoryLock(vAddress);

	if((pageDir[vAddress >> 22] & PAGE_FLAG_PRESENT) == 0) //check if page table is present
	{
		//if not, create one
		uintptr_t pageTableAddr;
		if(MM_PAGE_TABLE_SIZE != MmAllocatePhysicalMemory(MM_PAGE_TABLE_SIZE, &pageTableAddr))
		{
			MmFreePhysicalMemory(pageTableAddr, MM_PAGE_TABLE_SIZE);
			I686ReleaseMemoryLock(vAddress, prio);
			return MM_NO_MEMORY;
		}

		pageDir[vAddress >> 22] = pageTableAddr | PAGE_FLAG_WRITABLE | PAGE_FLAG_PRESENT; //store page directory entry
		I686_INVALIDATE_TLB((uintptr_t)(&PAGETABLE(vAddress >> 22, 0)));
		//now the table can be accessed

		for(uint16_t i = 0; i < MM_PAGE_TABLE_ENTRY_COUNT; i++)
		{
			PAGETABLE(vAddress >> 22, i) = 0; //clear page table
		}

		//from now on the page table can be accessed using pageTable[]
	}
	

	if(PAGETABLE(vAddress >> 22, (vAddress >> 12) & 0x3FF) & PAGE_FLAG_PRESENT) //check if page is already present
	{
		I686ReleaseMemoryLock(vAddress, prio);
		return MM_ALREADY_MAPPED;
	}

	uint16_t f = 0;
	if((flags & MM_FLAG_WRITABLE) && !(flags & MM_FLAG_READ_ONLY))
		f |= PAGE_FLAG_WRITABLE;
	if(flags & MM_FLAG_USER_MODE)
		f |= PAGE_FLAG_USER;
	if(flags & MM_FLAG_WRITE_THROUGH)
		f |= PAGE_FLAG_PWT;
	if(flags & MM_FLAG_CACHE_DISABLE)
		f |= PAGE_FLAG_PCD;
	PAGETABLE(vAddress >> 22, (vAddress >> 12) & 0x3FF) = (pAddress & 0xFFFFF000) | f | PAGE_FLAG_PRESENT; //add page to page table

	I686_INVALIDATE_TLB(vAddress); //invalidate old entry in TLB

	I686ReleaseMemoryLock(vAddress, prio);
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

static STATUS HalUnmapMemoryNoLock(uintptr_t vAddress)
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

	I686_INVALIDATE_TLB(vAddress); //invalidate old entry in TLB

	return OK;	
}

STATUS HalUnmapMemory(uintptr_t vAddress)
{
	PRIO prio = I686AcquireMemoryLock(vAddress);
	STATUS status = HalUnmapMemoryNoLock(vAddress);
#ifdef SMP
	struct KeTaskControlBlock *task;
	if(IS_KERNEL_MEMORY(vAddress))
		I686SendInvalidateKernelTlb(vAddress, 1);
	else if(NULL != (task = KeGetCurrentTask()))
	{
		I686SendInvalidateTlb(&(task->affinity), task->cpu.cr3, vAddress, 1);
	}
#endif
	I686ReleaseMemoryLock(vAddress, prio);
	return status;
}

STATUS HalUnmapMemoryEx(uintptr_t vAddress, uintptr_t size)
{
	size = ALIGN_UP(size, MM_PAGE_SIZE);
	uintptr_t start = vAddress;
	uintptr_t originalSize = size;
	PRIO prio = I686AcquireMemoryLock(vAddress);
	while(size)
	{
		HalUnmapMemoryNoLock(vAddress);
		vAddress += MM_PAGE_SIZE;
		size -= MM_PAGE_SIZE;
	}
#ifdef SMP
	vAddress = start;
	size = originalSize;

	struct KeTaskControlBlock *task = KeGetCurrentTask();

	while(size)
	{
		uintptr_t base = vAddress;
		uintptr_t sameTypeSize = 0;
		if(IS_KERNEL_MEMORY(vAddress))
		{
			do
			{
				sameTypeSize += MM_PAGE_SIZE;
				vAddress += MM_PAGE_SIZE;
				size -= MM_PAGE_SIZE;
			} 
			while(IS_KERNEL_MEMORY(vAddress) && (0 != size));
		}
		else
		{
			do
			{
				sameTypeSize += MM_PAGE_SIZE;
				vAddress += MM_PAGE_SIZE;
				size -= MM_PAGE_SIZE;
			} 
			while(!IS_KERNEL_MEMORY(vAddress) && (0 != size));			
		}

		if(IS_KERNEL_MEMORY(base))
			I686SendInvalidateKernelTlb(base, sameTypeSize / MM_PAGE_SIZE);
		else if(NULL != task)
			I686SendInvalidateTlb(&(task->affinity), task->cpu.cr3, base, sameTypeSize / MM_PAGE_SIZE);
	}
#endif
	I686ReleaseMemoryLock(start, prio);
	return OK;
}

void I686InitVirtualAllocator(void)
{
	GdtInit();
	/*
	The bootstrap code in boot.asm sets up initial paging that includes:
	1. Full kernel image mapping - so we can execute the kernel code
	2. Self-referencing page directory trick - so we can access the paging structures
	3. 1:1 bootstrap code mapping - which is not needed anymore, but it sits below the kernel space anyway
	When creating a process, only the kernel space mappings are copied, so no need to worry about point 3

	However, we want to create all kernel page tables, so that we don't have to propagate a page table creation
	from one CPU to the others.
	*/
	for(uintptr_t i = MM_KERNEL_SPACE_START; 
		i < MM_FIRST_PAGE_TABLE_ADDRESS; 
		i += (MM_PAGE_SIZE * MM_PAGE_DIRECTORY_ENTRY_COUNT))
	{
		if(pageDir[i >> 22] & PAGE_FLAG_PRESENT)
			continue;

		uintptr_t pta = allocatePageTable();
		if(0 == pta)
			FAIL_BOOT("page table allocation failed");
		pageDir[i >> 22] = pta | PAGE_FLAG_WRITABLE | PAGE_FLAG_PRESENT;
		I686_INVALIDATE_TLB((uintptr_t)(&PAGETABLE(i >> 22, 0)));
		//now the table can be accessed
		CmMemset(&PAGETABLE(i >> 22, 0), 0, MM_PAGE_TABLE_SIZE);
	}
}

uintptr_t I686GetPageDirectoryAddress(void)
{
	uintptr_t pageDir;
	ASM("mov eax,cr3" : "=a" (pageDir) : : );
	return pageDir;
}

STATUS I686CreateProcessMemorySpace(PADDRESS *pdAddress, uintptr_t stackAddress, void **stack)
{
	MmPageDirectoryEntry *pd = NULL;
	MmPageTableEntry *pt = NULL;
	PADDRESS ptAddress = 0;
	PADDRESS stackPhysical = 0;
	*pdAddress = 0;
	*stack = NULL;

	*pdAddress = allocatePageDirectory();
	ptAddress = allocatePageTable();
	if((0 == pdAddress) || (0 == ptAddress)
	|| (0 == MmAllocatePhysicalMemory(MM_PAGE_SIZE, &stackPhysical)))
		goto I686CreateProcessMemorySpaceFailure;
	
	pd = MmMapDynamicMemory(*pdAddress, MM_PAGE_DIRECTORY_SIZE, 0);
	if(NULL == pd)
		goto I686CreateProcessMemorySpaceFailure;

	pt = MmMapDynamicMemory(ptAddress, MM_PAGE_TABLE_SIZE, 0);
	if(NULL == pt)
		goto I686CreateProcessMemorySpaceFailure;

	CmMemset(pd, 0, MM_PAGE_DIRECTORY_SIZE);
	CmMemset(pt, 0, MM_PAGE_TABLE_SIZE);

	//copy entries for kernel space
	CmMemcpy(&pd[MM_KERNEL_SPACE_START >> 22], 
		&pageDir[MM_KERNEL_SPACE_START >> 22], 
		((MM_KERNEL_SPACE_SIZE >> 22) - 1) * sizeof(MmPageDirectoryEntry));
	
	//apply self-referencing page directory trick
	pd[MM_PAGE_DIRECTORY_ENTRY_COUNT - 1] = *pdAddress | PAGE_FLAG_WRITABLE | PAGE_FLAG_PRESENT;
	//store page table address is page directory
	pd[(stackAddress - MM_PAGE_SIZE) >> 22] = ptAddress | PAGE_FLAG_WRITABLE | PAGE_FLAG_PRESENT;
	pt[((stackAddress - MM_PAGE_SIZE) >> 12) & 0x3FF] = stackPhysical | PAGE_FLAG_WRITABLE | PAGE_FLAG_PRESENT;

	*stack = MmMapDynamicMemory(stackPhysical, MM_PAGE_SIZE, 0);
	if(NULL != *stack)
	{
		*stack = (void*)((uintptr_t)*stack + MM_PAGE_SIZE);
		MmUnmapDynamicMemory(pt);
		MmUnmapDynamicMemory(pd);
		return OK;
	}

I686CreateProcessMemorySpaceFailure:
	MmUnmapDynamicMemory(*stack);
	MmUnmapDynamicMemory(pt);
	MmUnmapDynamicMemory(pd);
	if(0 != ptAddress)
		MmFreePhysicalMemory(ptAddress, MM_PAGE_TABLE_SIZE);
	if(0 != pdAddress)
		MmFreePhysicalMemory(*pdAddress, MM_PAGE_DIRECTORY_SIZE);
	*pdAddress = 0;
	*stack = NULL;
	return OUT_OF_RESOURCES;
}

STATUS I686CreateThreadKernelStack(const struct KeTaskControlBlock *parent, uintptr_t stackAddress, void **stack)
{
	MmPageDirectoryEntry *pd = NULL;
	MmPageTableEntry *pt = NULL;
	PADDRESS ptAddress = 0;
	PADDRESS stackPhysical = 0;
	*stack = NULL;
	bool newPageTable = false;

	if(0 == MmAllocatePhysicalMemory(MM_PAGE_SIZE, &stackPhysical))
		goto I686CreateThreadKernelStackFailure;
	
	pd = MmMapDynamicMemory(parent->cpu.cr3, MM_PAGE_DIRECTORY_SIZE, 0);
	if(NULL == pd)
		goto I686CreateThreadKernelStackFailure;

	if(0 == (pd[(stackAddress - MM_PAGE_SIZE) >> 22] & PAGE_FLAG_PRESENT)) //page table for given stack address not present?
	{
		//create new page table
		ptAddress = allocatePageTable();
		if(0 == ptAddress)
			goto I686CreateThreadKernelStackFailure;
		newPageTable = true;
	}
	else
		ptAddress = pd[(stackAddress - MM_PAGE_SIZE) >> 22] & 0xFFFFF000;

	pt = MmMapDynamicMemory(ptAddress, MM_PAGE_TABLE_SIZE, 0);
	if(NULL == pt)
		goto I686CreateThreadKernelStackFailure;

	if(newPageTable)
	{
		pd[(stackAddress - MM_PAGE_SIZE) >> 22] = ptAddress | PAGE_FLAG_WRITABLE | PAGE_FLAG_PRESENT;
		CmMemset(pt, 0, MM_PAGE_TABLE_SIZE);
	}

	pt[((stackAddress - MM_PAGE_SIZE) >> 12) & 0x3FF] = stackPhysical | PAGE_FLAG_WRITABLE | PAGE_FLAG_PRESENT;

	*stack = MmMapDynamicMemory(stackPhysical, MM_PAGE_SIZE, 0);
	if(NULL != *stack)
	{
		*stack = (void*)((uintptr_t)*stack + MM_PAGE_SIZE);
		MmUnmapDynamicMemory(pt);
		MmUnmapDynamicMemory(pd);
		return OK;
	}

I686CreateThreadKernelStackFailure:
	MmUnmapDynamicMemory(*stack);
	MmUnmapDynamicMemory(pt);
	MmUnmapDynamicMemory(pd);
	if(0 != ptAddress)
		MmFreePhysicalMemory(ptAddress, MM_PAGE_TABLE_SIZE);
	*stack = NULL;
	return OUT_OF_RESOURCES;
}

//TODO: when mapping kernel space memory, TLBs must be invalidated across all processors
// STATUS HalMapForeignMemory(struct KeTaskControlBlock *target, uintptr_t vAddress, uintptr_t pAddress, MmMemoryFlags flags)
// {
// 	STATUS status = OK;
// 	MmPageTableEntry *pt = NULL;
// 	MmPageDirectoryEntry *pd = NULL;
// 	PADDRESS ptAddr = 0;
// 	bool allocatePageTable = false;

// 	if((target == KeGetCurrentTask())
// 		|| (vAddress >= MM_KERNEL_SPACE_START))
// 		return HalMapMemory(vAddress, pAddress, flags);

// 	pd = MmMapDynamicMemory(target->cpu.cr3, MM_PAGE_DIRECTORY_SIZE, 0);
// 	if(NULL == pd)
// 		return OUT_OF_RESOURCES;

// 	PRIO prio = KeAcquireSpinlock(target->cpu.userMemoryLock);

// 	if((pd[vAddress >> 22] & PAGE_FLAG_PRESENT) == 0) //page table is not present
// 	{
// 		allocatePageTable = true;
// 		//if not, create one
// 		if(MM_PAGE_TABLE_SIZE != MmAllocatePhysicalMemory(MM_PAGE_TABLE_SIZE, &ptAddr))
// 		{
// 			status = OUT_OF_RESOURCES;
// 			goto HalMapForeignMemoryExit;
// 		}
// 	}
// 	else //page table is present
// 	{
// 		ptAddr = pd[vAddress >> 22] & 0xFFFFF000;
// 	}

// 	//map page table
// 	pt = MmMapDynamicMemory(ptAddr, MM_PAGE_TABLE_SIZE, 0);
// 	if(NULL == pt)
// 	{
// 		status = OUT_OF_RESOURCES;
// 		goto HalMapForeignMemoryExit;
// 	}

// 	if(allocatePageTable)
// 	{
// 		CmMemset(pt, 0, MM_PAGE_TABLE_SIZE);
// 		pd[vAddress >> 22] = ptAddr | PAGE_FLAG_WRITABLE | PAGE_FLAG_PRESENT; //store page directory entry
// #ifdef SMP
// 		//TODO: send invalidate tlb ipi to other processor
// #endif
// 	}

// 	if(pt[(vAddress >> 12) & 0x3FF] & PAGE_FLAG_PRESENT) //check if page is already present
// 	{
// 		status = MM_ALREADY_MAPPED;
// 		goto HalMapForeignMemoryExit;
// 	}

// 	uint16_t f = 0;
// 	if(flags & MM_FLAG_WRITABLE)
// 		f |= PAGE_FLAG_WRITABLE;
// 	if(flags & MM_FLAG_USER_MODE)
// 		f |= PAGE_FLAG_USER;
// 	if(flags & MM_FLAG_WRITE_THROUGH)
// 		f |= PAGE_FLAG_PWT;
// 	if(flags & MM_FLAG_CACHE_DISABLE)
// 		f |= PAGE_FLAG_PCD;
// 	PAGETABLE(vAddress >> 22, (vAddress >> 12) & 0x3FF) = (pAddress & 0xFFFFF000) | f | PAGE_FLAG_PRESENT; //add page to page table

// 	I686_INVALIDATE_TLB(vAddress); //invalidate old entry in TLB

// HalMapForeignMemoryExit:

// 	return status;	
// }

#endif