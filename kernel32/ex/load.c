#include "load.h"
#include "elf.h"
#include "io/fs/fs.h"
#include "mm/mm.h"
#include "ke/sched/sched.h"

// STATUS ElfAllocateBss(struct Elf32_Ehdr *h)
// {
// 	struct Elf32_Shdr *sectionHdr;

// 	for(uint16_t i = 0; i < h->e_shnum; i++) //iterate through all sections
// 	{
// 		sectionHdr = ElfGetSectionHeader(h, i); //get header

// 		if(SHT_NOBITS != sectionHdr->sh_type) //process only no-bits/bss sections
// 			continue;
		
// 		if(0 == sectionHdr->sh_size) //skip empty
// 			continue;

// 		if(sectionHdr->sh_flags & SHF_ALLOC) //check if memory for this section should be allocated in memory
// 		{
// 			void *m = MmAllocateKernelHeap(sectionHdr->sh_size);
// 			if(NULL == m)
// 				return OUT_OF_RESOURCES;
// 			CmMemset(m, 0, sectionHdr->sh_size);
// 			//store offset between allocated memory and ELF header in sh_offset
// 			sectionHdr->sh_offset = (uintptr_t)m - (uintptr_t)h;
// 		}
// 	}
// 	return OK;
// }

void ExProcessLoadWorker(char *path)
{
    // if(!IoCheckIfFileExists(path))
	// {
    //     return;
	// }
	
	// struct KeTaskControlBlock *tcb = KeGetCurrentTask();

    // uint64_t imageSize, freeSize;
    // uintptr_t address = MM_PAGE_SIZE;
	// STATUS ret = OK;
    // if(OK != (ret = IoGetFileSize(path, &imageSize)))
    // {
	// 	tcb->finishReason = ret;
	// 	return;
	// }

    // freeSize = KeGetHighestAvailableMemory() - address;
    // IoFileHandle *f = NULL;
    // if((OK != (ret = IoOpenKernelFile(path, IO_FILE_READ | IO_FILE_BINARY, 0, &f)) || (NULL == f)))
    // {
	// 	tcb->finishReason = ret;
	// 	return;
	// }
    
    // if(OK != (ret = MmAllocateMemory(address, imageSize, ((PL_USER == tcb->pl) ? MM_PAGE_FLAG_USER : 0) | MM_PAGE_FLAG_WRITABLE)))
    // {
    //     IoCloseKernelFile(f);
	// 	tcb->finishReason = ret;
    //     return;
    // }

	// uint64_t actualSize = 0;
	// if((OK != (ret = IoReadKernelFile(f, (void*)address, imageSize, 0, &actualSize))) || (actualSize != imageSize))
	// {
	// 	IoCloseKernelFile(f);
	// 	MmFreeMemory(address, imageSize);
	// 	if(OK != ret)
	// 		tcb->finishReason = ret;
	// 	else
	// 		tcb->finishReason = IO_READ_INCOMPLETE;
	// 	return;
	// }

	// IoCloseKernelFile(f);

	// struct Elf32_Ehdr *elfHeader = (struct Elf32_Ehdr*)address;
	// if(OK != (ret = ExVerifyElf32Header(elfHeader)))
	// {
	// 	MmFreeMemory(address, imageSize);
	// 	tcb->finishReason = ret;
	// 	return;
	// }

	// // if(PL_USER == tcb->pl)
	// // {
	// // 	if(TYPE_REL elfHeader->e_type)
	// // }

}