#include "load.h"
#include "elf.h"
#include "io/fs/fs.h"
#include "mm/tmem.h"
#include "mm/heap.h"
#include "ke/sched/sched.h"
#include "rtl/string.h"
#include "rtl/stdlib.h"

static STATUS ExLoadImage(const char *path, bool isInterpreter, void (**entry)(void*), uintptr_t *imageTop, struct ExProgramData **progData)
{
	STATUS status = OK;
	int f = -1;
	struct Elf32_Ehdr *ehdr = NULL;
	struct Elf32_Phdr *phdr = NULL;
	char *interpreter = nullptr;
	size_t actualSize = 0;
	bool interpreterFound = false;

	*imageTop = 0;

	//randomize image base by 9 bits = 512 position on a page granularity
	uintptr_t location = RtlRandom(1, 1 << 9) * PAGE_SIZE;

    if(!IoCheckIfFileExists(path))
	{
        return NOT_FOUND;
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
		status = OPERATION_INCOMPLETE;
		goto ExProcessLoadWorkerFailed;
	}

	status = ExVerifyElf32Header(ehdr);
	if(OK != status)
		goto ExProcessLoadWorkerFailed;

	if((ET_DYN != ehdr->e_type) || (nullptr == (void*)ehdr->e_entry))
	{
		status = BAD_TYPE;
		goto ExProcessLoadWorkerFailed;
	}

	size_t phdrSize = ehdr->e_phentsize * ehdr->e_phnum;
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
		status = OPERATION_INCOMPLETE;
		goto ExProcessLoadWorkerFailed;
	}

	for(uint16_t i = 0; i < ehdr->e_phnum; i++)
	{
		if(PT_INTERP == phdr[i].p_type)
		{
			if(!isInterpreter)
			{
				interpreter = MmAllocateKernelHeap(phdr[i].p_filesz);
				if(nullptr == interpreter)
				{
					status = OUT_OF_RESOURCES;
					goto ExProcessLoadWorkerFailed;
				}
				status = IoReadFileSync(f, interpreter, phdr[i].p_filesz, phdr[i].p_offset, &actualSize);
				if(OK != status)
					goto ExProcessLoadWorkerFailed;
				else if(actualSize < phdr[i].p_filesz)
				{
					status = OPERATION_INCOMPLETE;
					goto ExProcessLoadWorkerFailed;
				}

				if('\0' != interpreter[phdr[i].p_filesz])
				{
					status = CORRUPTED;
					goto ExProcessLoadWorkerFailed;
				}

				status = ExLoadImage(interpreter, true, entry, imageTop, progData);
				if(OK != status)
					goto ExProcessLoadWorkerFailed;

				interpreterFound = true;
			}
			else
			{
				//disallow interpreter chains
				status = NOT_SUPPORTED;
				goto ExProcessLoadWorkerFailed;
			}
			break;
		}
	}

	if(!isInterpreter && !interpreterFound)
	{
		status = NOT_SUPPORTED;
		goto ExProcessLoadWorkerFailed;
	}

	if(!isInterpreter)
	{
		*progData = MmAllocateKernelHeap(sizeof(struct ExProgramData) * 4);
		if(nullptr == *progData)
		{
			status = OUT_OF_RESOURCES;
			goto ExProcessLoadWorkerFailed;
		}

		progData[0]->type = PROGDATA_BASE;
		progData[0]->value.p = (void*)location;
		progData[1]->type = PROGDATA_PAGE_SIZE;
		progData[1]->value.s = PAGE_SIZE;
		progData[2]->type = PROGDATA_END;
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

			uintptr_t base = location + ALIGN_DOWN(phdr[i].p_vaddr, PAGE_SIZE);
			uintptr_t top = (0 != phdr[i].p_filesz) ?
				ALIGN_UP(location + phdr[i].p_vaddr + phdr[i].p_filesz, PAGE_SIZE)
				: ALIGN_UP(location + phdr[i].p_vaddr + phdr[i].p_memsz, PAGE_SIZE);
			enum MmTaskMemoryFlags flags = MM_TASK_MEMORY_FIXED | MM_TASK_MEMORY_LOCKED;
			if(phdr[i].p_flags & PF_R)
				flags |= MM_TASK_MEMORY_READABLE;
			if(phdr[i].p_flags & PF_W)
				flags |= MM_TASK_MEMORY_WRITABLE;
			if(phdr[i].p_flags & PF_X)
				flags |= MM_TASK_MEMORY_EXECUTABLE;

			if(0 != phdr[i].p_filesz)
			{
				status = MmMapTaskMemory((void*)base, top - base, flags, f, 0, ALIGN_DOWN(phdr[i].p_offset, PAGE_SIZE), 0, NULL);
				if(OK != status)
				{
					goto ExProcessLoadWorkerFailed;
				}
				base = top;
				top = ALIGN_UP(location + phdr[i].p_vaddr + phdr[i].p_memsz, PAGE_SIZE);
			}

			if(top != base)
			{
				status = MmMapTaskMemory((void*)base, top - base, flags, -1, 0, 0, 0, NULL);
				if(OK != status)
				{
					goto ExProcessLoadWorkerFailed;
				}
			}

			if(top > *imageTop)
				*imageTop = top;
		}
	}

	if(isInterpreter || !interpreterFound)
		*entry = (void(*)(void*))(ehdr->e_entry);

ExProcessLoadWorkerFailed:
	if(f >= 0)
		IoCloseFile(f);
	MmFreeKernelHeap(ehdr);
	MmFreeKernelHeap(phdr);
	MmFreeKernelHeap(interpreter);
	MmFreeKernelHeap(*progData);

	return status;
}

STATUS ExLoadProcessImage(const char *path, void (**entry)(void*), struct ExProgramData **progData)
{
	STATUS status = OK;
	uintptr_t imageTop = 0;
	struct KeTaskControlBlock *tcb = KeGetCurrentTask();

	status = ExLoadImage(path, false, entry, &imageTop, progData);
	if(OK == status)
	{
		//randomize dynamic memory base (9 bits giving 512 positions)
		int32_t location = RtlRandom(0, 1 << 9);
		KeAcquireMutex(&(tcb->parent->memory.mutex));
		//calculate dynamic memory base with page granularity
		tcb->parent->memory.base = (void*)(ALIGN_UP(imageTop, PAGE_SIZE) + location * PAGE_SIZE);
		KeReleaseMutex(&(tcb->parent->memory.mutex));
	}

	return status;
}

size_t ExGetProgramDataEntryCount(const struct ExProgramData *progData)
{
	size_t count = 1;
	while(PROGDATA_END != progData->type)
	{
		++progData;
		++count;
	}

	return count;
}