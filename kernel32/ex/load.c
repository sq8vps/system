#include "load.h"
#include "elf.h"
#include "io/fs/fs.h"
#include "mm/tmem.h"
#include "mm/heap.h"
#include "ke/sched/sched.h"
#include "rtl/string.h"

STATUS ExLoadProcessImage(const char *path, void (**entry)())
{
	STATUS status = OK;
	int f = -1;
	struct Elf32_Ehdr *ehdr = NULL;
	struct Elf32_Phdr *phdr = NULL;
	size_t actualSize = 0;

    if(!IoCheckIfFileExists(path))
	{
        return FILE_NOT_FOUND;
	}
	
    status = IoOpenFile(path, IO_FILE_READ, 0, &f);
	if(OK != status)
		return status;
    
    ehdr = MmAllocateKernelHeap(sizeof(*ehdr));
	if(NULL == ehdr)
	{
		status = OUT_OF_RESOURCES;
		goto ExProcessLoadWorkerFailed;
	}

	status = IoReadFileSync(f, ehdr, sizeof(*ehdr), 0, &actualSize);
	if(OK != status)
		goto ExProcessLoadWorkerFailed;
	else if(actualSize < sizeof(*ehdr))
	{
		status = READ_INCOMPLETE;
		goto ExProcessLoadWorkerFailed;
	}

	status = ExVerifyElf32Header(ehdr);
	if(OK != status)
		goto ExProcessLoadWorkerFailed;

	if(ET_EXEC != ehdr->e_type)
	{
		status = ELF_UNSUPPORTED_TYPE;
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

	status = IoReadFileSync(f, phdr, phdrSize, ehdr->e_phoff, &actualSize);
	if(OK != status)
		goto ExProcessLoadWorkerFailed;
	else if(actualSize < phdrSize)
	{
		status = READ_INCOMPLETE;
		goto ExProcessLoadWorkerFailed;
	}

	for(uint16_t i = 0; i < ehdr->e_phnum; ++i)
	{
		if(PT_LOAD == phdr[i].p_type)
		{
			if((phdr[i].p_offset & (PAGE_SIZE - 1)) != (phdr[i].p_vaddr & (PAGE_SIZE - 1)))
			{
				status = BAD_ALIGNMENT;
				goto ExProcessLoadWorkerFailed;
			}

			uintptr_t base = ALIGN_DOWN(phdr[i].p_vaddr, PAGE_SIZE);
			uintptr_t top = ALIGN_UP(phdr[i].p_vaddr + phdr[i].p_memsz, PAGE_SIZE);
			enum MmTaskMemoryFlags flags = MM_TASK_MEMORY_FIXED | MM_TASK_MEMORY_LOCKED;
			if(phdr[i].p_flags & PF_R)
				flags |= MM_TASK_MEMORY_READABLE;
			if(phdr[i].p_flags & PF_W)
				flags |= MM_TASK_MEMORY_WRITABLE;
			if(phdr[i].p_flags & PF_X)
				flags |= MM_TASK_MEMORY_EXECUTABLE;
			
			status = MmMapTaskMemory((void*)base, top - base, flags, f, ALIGN_DOWN(phdr[i].p_offset, PAGE_SIZE), 0, NULL);
			if(OK != status)
			{
				goto ExProcessLoadWorkerFailed;
			}
		}
	}

	*entry = (void(*)())(ehdr->e_entry);

ExProcessLoadWorkerFailed:
	if(f >= 0)
		IoCloseFile(f);
	MmFreeKernelHeap(ehdr);
	MmFreeKernelHeap(phdr);

	return status;
}