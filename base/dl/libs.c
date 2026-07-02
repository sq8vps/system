#include "libs.h"
#include "elf.h"
#include <errno.h>
#include <mm/tmem.h>
#include <io/fs/fs.h>
#include <stdlib.h>
#include <string.h>

#define DL_DEFAULT_LIB_PATH "/main/system/lib/"

extern size_t DlPageSize;

struct DlState DlState = {.loaded = {.count = 0, .list = nullptr}};;

static STATUS DlLoadSharedLibrary(const char *name, char **envp, struct DlLibrary **l)
{
	STATUS status = OK;
	int f = -1;
	struct Elf32_Ehdr *ehdr = nullptr;
	struct Elf32_Phdr *phdr = nullptr;
	struct Elf32_Dyn *dyn = nullptr;
	struct DlLibrary *lib = nullptr;
	char *path = nullptr;
	size_t actualSize = 0;

	path = malloc(strlen(DL_DEFAULT_LIB_PATH) + strlen(name) + 1);
	if(nullptr == path)
	{
		status = OUT_OF_RESOURCES;
		goto leave;
	}

	strcpy(path, DL_DEFAULT_LIB_PATH);
	strcat(path, name);

	if(nullptr != DlState.loaded.list)
	{
		struct DlLibrary *t = DlState.loaded.list;
		while(nullptr != t)
		{
			if(0 == strcmp(t->path, path))
			{
				*l = t;
				return OK;
			}
			t = t->next;
		}
	}
	
    status = ApiOpenFile(path, IO_FILE_READ, 0, &f);
	if(OK != status)
		return status;

	lib = calloc(sizeof(*lib) + strlen(path) + 1, 1);
	if(nullptr == lib)
	{
		status = OUT_OF_RESOURCES;
		goto leave;
	}
    
    ehdr = malloc(sizeof(*ehdr));
	if(nullptr == ehdr)
	{
		status = OUT_OF_RESOURCES;
		goto leave;
	}

	status = ApiReadFileSync(f, ehdr, sizeof(*ehdr), 0, &actualSize);
	if(OK != status)
		goto leave;
	else if(actualSize < sizeof(*ehdr))
	{
		status = OPERATION_INCOMPLETE;
		goto leave;
	}

	status = DlVerifyElf32Header(ehdr);
	if(OK != status)
		goto leave;

	if(ET_DYN != ehdr->e_type)
	{
		status = BAD_TYPE;
		goto leave;
	}

	size_t phdrSize = ehdr->e_phentsize * ehdr->e_phnum;
    phdr = malloc(phdrSize);
	if(NULL == phdr)
	{
		status = OUT_OF_RESOURCES;
		goto leave;
	}

	status = ApiReadFileSync(f, phdr, phdrSize, ehdr->e_phoff, &actualSize);
	if(OK != status)
		goto leave;
	else if(actualSize < phdrSize)
	{
		status = OPERATION_INCOMPLETE;
		goto leave;
	}

	void *mapping = nullptr;
	uintptr_t lowestBase = UINTPTR_MAX;
	uintptr_t highestTop = 0;

	for(uint16_t i = 0; i < ehdr->e_phnum; ++i)
	{
		if(PT_LOAD == phdr[i].p_type)
		{
			if((phdr[i].p_offset & (DlPageSize - 1)) != (phdr[i].p_vaddr & (DlPageSize - 1)))
			{
				status = BAD_ALIGNMENT;
				goto leave;
			}

			uintptr_t base = ALIGN_DOWN(phdr[i].p_vaddr, DlPageSize);
			uintptr_t top = ALIGN_UP(phdr[i].p_vaddr + phdr[i].p_memsz, DlPageSize);
			if(base < lowestBase)
				lowestBase = base;

			if(top > highestTop)
				highestTop = top;
		}
	}

	status = ApiMapTaskMemoryA(nullptr, highestTop - lowestBase, 0, &mapping);
	if(OK != status)
		goto leave;

	for(uint16_t i = 0; i < ehdr->e_phnum; ++i)
	{
		if(PT_LOAD == phdr[i].p_type)
		{
			uintptr_t base = ALIGN_DOWN(phdr[i].p_vaddr, DlPageSize);
			uintptr_t top = (0 != phdr[i].p_filesz) ?
				ALIGN_UP(phdr[i].p_vaddr + phdr[i].p_filesz, DlPageSize)
				: ALIGN_UP(phdr[i].p_vaddr + phdr[i].p_memsz, DlPageSize);
			enum MmTaskMemoryFlags flags = MM_TASK_MEMORY_FIXED | MM_TASK_MEMORY_OVERRIDE;
			if(phdr[i].p_flags & PF_R)
				flags |= MM_TASK_MEMORY_READABLE;
			if(phdr[i].p_flags & PF_W)
				flags |= MM_TASK_MEMORY_WRITABLE;
			if(phdr[i].p_flags & PF_X)
				flags |= MM_TASK_MEMORY_EXECUTABLE;

			if(0 != phdr[i].p_filesz)
			{
				status = ApiMapTaskMemory(
					(void*)((uintptr_t)mapping + (base - lowestBase)),
					top - base, flags, f, 0, ALIGN_DOWN(phdr[i].p_offset, DlPageSize), 0, 
					nullptr);
				if(OK != status)
				{
					goto leave;
				}
				base = top;
				top = ALIGN_UP(phdr[i].p_vaddr + phdr[i].p_memsz, DlPageSize);
			}

			if(top != base)
			{
				status = ApiMapTaskMemoryA((void*)((uintptr_t)mapping + (base - lowestBase)), top - base, flags, nullptr);
				if(OK != status)
				{
					goto leave;
				}
			}

			if(phdr[i].p_filesz != phdr[i].p_memsz)
			{
				memset(
					(void*)((uintptr_t)mapping + (phdr[i].p_vaddr - lowestBase) + phdr[i].p_filesz), 
					0, 
					phdr[i].p_memsz - phdr[i].p_filesz
				);
			}
		}
	}

	for(uint16_t k = 0; k < ehdr->e_phnum; ++k)
	{
		if(PT_DYNAMIC == phdr[k].p_type)
		{
			uintptr_t shift = (uintptr_t)mapping - lowestBase;
			dyn = (struct Elf32_Dyn*)(phdr[k].p_vaddr + shift);
            for(size_t i = 0; i < (phdr[k].p_memsz / sizeof(struct Elf32_Dyn)); i++)
            {
                switch(dyn[i].d_tag)
                {
                    case DT_SYMTAB:
                        lib->dynSym = (struct Elf32_Sym*)(dyn[i].d_un.d_ptr + shift);
                        break;
					case DT_STRTAB:
						lib->dynStr = (char*)(dyn[i].d_un.d_ptr + shift);
						break;
					case DT_HASH:
						lib->hashTab = (void*)(dyn[i].d_un.d_ptr + shift);
						lib->dynSymCount = lib->hashTab->nchain;
						break;
					case DT_SYMTABSZ:
						lib->dynSymCount = dyn[i].d_un.d_val / sizeof(struct Elf32_Sym);
						break;
                    default:
                        break;
                }
            }
			break;
		}
	}

	strcpy(lib->path, path);
	lib->h = mapping;
	lib->ready = false;
	lib->next = nullptr;
	*l = lib;

	++DlState.loaded.count;
	if(nullptr != DlState.loaded.list)
	{
		struct DlLibrary *t = DlState.loaded.list;
		while(nullptr != t->next)
			t = t->next;
		t->next = lib;
	}
	else
		DlState.loaded.list = lib;

leave:
	if(f >= 0)
		ApiCloseFile(f);
	if(OK != status)
		free(lib);
	free(path);
	free(ehdr);
	free(phdr);
	return status;
}

STATUS DlLoadLibs(const struct Elf32_Ehdr *h, char **envp)
{
    STATUS status = OK;
    struct Elf32_Phdr *phdr = nullptr; //program data headers
    struct Elf32_Dyn *dyn = nullptr; //dynamic entries
    const char *strTab = nullptr; //string table

    status = DlVerifyElf32Header(h);
    if(OK != status)
        return status;

    phdr = (struct Elf32_Phdr*)(h->e_phoff + (uintptr_t)h);
    for(size_t i = 0; i < h->e_phnum; i++)
    {
        if(PT_DYNAMIC == phdr->p_type)
        {
            dyn = (struct Elf32_Dyn*)(phdr->p_vaddr + (uintptr_t)h);

            for(size_t i = 0; i < (phdr->p_memsz / sizeof(struct Elf32_Dyn)); i++)
            {
                if(DT_STRTAB == dyn[i].d_tag)
                {
                    strTab = (const char*)(dyn[i].d_un.d_ptr + (uintptr_t)h);
                }
            }
            
            if(nullptr == strTab) //string table is mandatory according to specs
                return CORRUPTED;
            
            for(size_t i = 0; i < (phdr->p_memsz / sizeof(struct Elf32_Dyn)); i++)
            {
                if(DT_NEEDED == dyn[i].d_tag)
                {
                    struct DlLibrary *lib = nullptr;
                    status = DlLoadSharedLibrary(&strTab[dyn[i].d_un.d_val], envp, &lib);
                    if(OK != status)
						return status;

					if(!lib->ready) //avoid loading dependecies and relocating again
					{
						status = DlLoadLibs(lib->h, envp);
						if(OK != status)
							return status;

						status = DlPerformRelocations(lib->h);
						if(OK != status)
							return status;

						lib->ready = true;
					}
                }
            }
        }
        phdr = (struct Elf32_Phdr*)((uintptr_t)phdr + h->e_phentsize);
    }

    return OK;
}


STATUS DlInsertLoaderToList(const struct Elf32_Ehdr *progHdr, struct Elf32_Ehdr *dlHdr)
{
	STATUS status = OK;
	const char *path = nullptr;
	const struct Elf32_Phdr *phdr = (const struct Elf32_Phdr*)((uintptr_t)progHdr + progHdr->e_phoff);
	const struct Elf32_Dyn *dyn = nullptr;
	struct DlLibrary *lib = nullptr;

	for(uint16_t i = 0; i < progHdr->e_phnum; ++i)
	{
		if(PT_INTERP == phdr[i].p_type)
		{
			path = (const char *)((uintptr_t)progHdr + phdr[i].p_vaddr);
		}
	}

	if(nullptr == path) //Should not happen, but maybe don't treat this an error. This library will just be loaded again if needed.
		return OK;

	lib = calloc(sizeof(*lib) + strlen(path) + 1, 1);
	if(nullptr == lib)
	{
		status = OUT_OF_RESOURCES;
		goto leave;
	}

	strcpy(lib->path, path);
	lib->h = dlHdr;
	lib->ready = true;
	lib->next = nullptr;

	phdr = (const struct Elf32_Phdr*)((uintptr_t)dlHdr + dlHdr->e_phoff);
	for(uint16_t i = 0; i < dlHdr->e_phnum; ++i)
	{
		if(PT_DYNAMIC == phdr->p_type)
		{
			dyn = (struct Elf32_Dyn*)(phdr->p_vaddr + (uintptr_t)dlHdr);
            for(size_t i = 0; i < (phdr->p_memsz / sizeof(struct Elf32_Dyn)); i++)
            {
                switch(dyn[i].d_tag)
                {
                    case DT_SYMTAB:
                        lib->dynSym = (struct Elf32_Sym*)(dyn[i].d_un.d_ptr + (uintptr_t)dlHdr);
                        break;
					case DT_STRTAB:
						lib->dynStr = (char*)(dyn[i].d_un.d_ptr + (uintptr_t)dlHdr);
						break;
					case DT_HASH:
						lib->hashTab = (void*)(dyn[i].d_un.d_ptr + (uintptr_t)dlHdr);
						lib->dynSymCount = lib->hashTab->nchain;
						break;
					case DT_SYMTABSZ:
						lib->dynSymCount = dyn[i].d_un.d_val / sizeof(struct Elf32_Sym);
						break;
                    default:
                        break;
                }
            }
		}
		++phdr;
	}

	++DlState.loaded.count;
	if(nullptr != DlState.loaded.list)
	{
		struct DlLibrary *t = DlState.loaded.list;
		while(nullptr != t->next)
			t = t->next;
		t->next = lib;
	}
	else
		DlState.loaded.list = lib;

leave:
	if(OK != status)
		free(lib);
	return status;
}