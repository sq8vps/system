#include "exec.h"
#include "ke/task/task.h"
#include "ke/sched/sched.h"
#include "io/fs/fs.h"
#include "mm/heap.h"
#include "elf.h"
#include "common.h"

STATUS ExCreateProcess(char *name, char *path, PrivilegeLevel_t pl, struct KeTaskControlBlock **tcb)
{
    if((NULL == path) || (NULL == name))
        return NULL_POINTER_GIVEN;
    
    STATUS ret = OK;
    
    if(!IoCheckIfFileExists(path))
        return IO_FILE_NOT_FOUND;
    
    if(OK != (ret = KeCreateProcess(name, path, pl, tcb)))
        return ret;
    
    return KeEnableTask(*tcb);
}

STATUS ExGetExecutableRequiredBssSize(const char *name, uintptr_t *size)
{
    struct IoFileHandle *f;
    uint64_t actualSize;
    STATUS ret = OK;

    *size = 0;

    if(OK != (ret = IoOpenKernelFile((char*)name, IO_FILE_READ | IO_FILE_BINARY, 0, &f)))
    {
        return ret;
    }

    uint8_t *buf = MmAllocateKernelHeap(sizeof(struct Elf32_Ehdr));
    if(NULL == buf)
    {
        IoCloseKernelFile(f);
        return OUT_OF_RESOURCES;
    }

    if((OK != (ret = IoReadKernelFileSync(f, buf, sizeof(struct Elf32_Ehdr), 0, &actualSize)) || (actualSize != (uint64_t)sizeof(struct Elf32_Ehdr))))
    {
        MmFreeKernelHeap(buf);
        IoCloseKernelFile(f);
        if(OK != ret)
            return ret;
        else
            return IO_READ_INCOMPLETE;
    }
    struct Elf32_Ehdr *h = (struct Elf32_Ehdr*)buf;
    if(OK != (ret = ExVerifyElf32Header(h)))
    {
        MmFreeKernelHeap(buf);
        IoCloseKernelFile(f);
        return ret;
    }

    uint32_t sectionHeaderEntryCount = h->e_shnum;
    uint64_t sectionHeaderSize = h->e_shnum * h->e_shentsize;
    uint32_t sectionHeaderOffset = h->e_shoff;

    MmFreeKernelHeap(buf);
    buf = MmAllocateKernelHeap(sectionHeaderSize);
    if(NULL == buf)
    {
        IoCloseKernelFile(f);
        return OUT_OF_RESOURCES;
    }

    if((OK != (ret = IoReadKernelFileSync(f, buf, sectionHeaderSize, sectionHeaderOffset, &actualSize)) || (actualSize != sectionHeaderSize)))
    {
        MmFreeKernelHeap(buf);
        IoCloseKernelFile(f);
        if(OK != ret)
            return ret;
        else
            return IO_READ_INCOMPLETE;
    }
    IoCloseKernelFile(f);

    struct Elf32_Shdr *s = (struct Elf32_Shdr*)buf;
	for(uint16_t i = 0; i < sectionHeaderEntryCount; i++)
	{
		if(SHT_NOBITS != s[i].sh_type)
			continue;
		
		if(0 == s[i].sh_size) 
			continue;

		if(s[i].sh_flags & SHF_ALLOC)
		{
			*size += s[i].sh_size;
		}
	}
    MmFreeKernelHeap(buf);
    return OK;
}

STATUS ExPrepareExecutableBss(void *fileStart, void *bss)
{
    STATUS ret = OK;

    struct Elf32_Ehdr *h = fileStart;
    if(OK != (ret = ExVerifyElf32Header(h)))
        return ret;
    
    struct Elf32_Shdr *s;
	for(uint16_t i = 0; i < h->e_shnum; i++)
	{
        s = ExGetElf32SectionHeader(h, i);
		if(SHT_NOBITS != s->sh_type)
			continue;
		
		if(0 == s->sh_size) 
			continue;

		if(s->sh_flags & SHF_ALLOC)
		{
			CmMemset(bss, 0, s->sh_size);
            s->sh_offset = (uintptr_t)bss - (uintptr_t)fileStart;
            bss = (void*)((uintptr_t)bss + (uintptr_t)s->sh_size);
		}
	}
    return OK;
}