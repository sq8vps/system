#include "load.h"
#include "elf.h"
#include "io/fs/fs.h"
#include "mm/mm.h"
#include "ke/sched/sched.h"
#include "rtl/string.h"

STATUS ExLoadProcessImage(const char *path, void (**entry)())
{
	STATUS status = OK;
	IoFileHandle *f = NULL;
	struct Elf32_Ehdr *ehdr = NULL;
	struct Elf32_Phdr *phdr = NULL;
	uint64_t actualSize = 0;

    if(!IoCheckIfFileExists(path))
	{
        return IO_FILE_NOT_FOUND;
	}
	
	struct KeTaskControlBlock *tcb = KeGetCurrentTask();
    
    status = IoOpenKernelFile(path, IO_FILE_READ, 0, &f);
	if(OK != status)
		return status;
    
    ehdr = MmAllocateKernelHeap(sizeof(struct Elf32_Ehdr));
	if(NULL == ehdr)
	{
		status = OUT_OF_RESOURCES;
		goto ExProcessLoadWorkerFailed;
	}

	status = IoReadKernelFileSync(f, ehdr, sizeof(struct Elf32_Ehdr), 0, &actualSize);
	if(OK != status)
		goto ExProcessLoadWorkerFailed;
	else if(actualSize < sizeof(struct Elf32_Ehdr))
	{
		status = IO_READ_INCOMPLETE;
		goto ExProcessLoadWorkerFailed;
	}

	status = ExVerifyElf32Header(ehdr);
	if(OK != status)
		goto ExProcessLoadWorkerFailed;

	if(ET_EXEC != ehdr->e_type)
	{
		status = EXEC_ELF_UNSUPPORTED_TYPE;
		goto ExProcessLoadWorkerFailed;
	}
	//TODO: implement PIE handling

	uint32_t phdrSize = ehdr->e_phentsize * ehdr->e_phnum;
    phdr = MmAllocateKernelHeap(phdrSize);
	if(NULL == phdr)
	{
		status = OUT_OF_RESOURCES;
		goto ExProcessLoadWorkerFailed;
	}

	status = IoReadKernelFileSync(f, phdr, phdrSize, ehdr->e_phoff, &actualSize);
	if(OK != status)
		goto ExProcessLoadWorkerFailed;
	else if(actualSize < phdrSize)
	{
		status = IO_READ_INCOMPLETE;
		goto ExProcessLoadWorkerFailed;
	}

	for(uint16_t i = 0; i < ehdr->e_phnum; ++i)
	{
		if(PT_LOAD == phdr[i].p_type)
		{
			if(phdr[i].p_vaddr + phdr[i].p_memsz > ((uintptr_t)tcb->stack.user.top - tcb->stack.user.size))
			{
				status = OUT_OF_RESOURCES;
				goto ExProcessLoadWorkerFailed;
			}
			uintptr_t base = ALIGN_DOWN(phdr[i].p_vaddr, PAGE_SIZE);
			uintptr_t top = ALIGN_UP(base + phdr[i].p_memsz, PAGE_SIZE);
			uint16_t flags = MM_FLAG_USER_MODE;
			if(phdr[i].p_flags & PF_W)
				flags |= MM_FLAG_WRITABLE;
			else
				flags |= MM_FLAG_READ_ONLY;
			if(phdr[i].p_flags & PF_X)
				flags |= MM_FLAG_EXECUTABLE;
			status = MmAllocateMemory(base, top - base, flags);
			if(OK != status)
				goto ExProcessLoadWorkerFailed;
			
			RtlMemset((void*)base, 0, top - base);

			status = IoReadKernelFileSync(f, (void*)phdr[i].p_vaddr, phdr[i].p_filesz, phdr[i].p_offset, &actualSize);
			if(OK != status)
			{
				goto ExProcessLoadWorkerFailed;
			}
			else if(phdr[i].p_filesz != actualSize)
			{
				status = IO_READ_INCOMPLETE;
				goto ExProcessLoadWorkerFailed;
			}
		}
	}

	*entry = (void(*)())(ehdr->e_entry);

ExProcessLoadWorkerFailed:
	if(NULL != f)
		IoCloseKernelFile(f);
	MmFreeKernelHeap(ehdr);
	MmFreeKernelHeap(phdr);

	return status;
}