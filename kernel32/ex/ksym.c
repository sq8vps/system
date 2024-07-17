#include "ksym.h"
#include "elf.h"
#include "mm/heap.h"
#include "io/fs/fs.h"
#include "common.h"

struct KernelSymbol
{
	uintptr_t value;
	char *name;
} PACKED;

static char *kernelSymbolStrings = NULL;
static struct KernelSymbol *kernelSymbolTable = NULL;
static uint32_t kernelSymbolCount = 0;

STATUS ExLoadKernelSymbols(char *path)
{
    STATUS ret = OK;
    IoFileHandle *f;
    if(OK != (ret = IoOpenKernelFile(path, IO_FILE_READ | IO_FILE_BINARY, 0, &f)))
        return ret;
    
    struct Elf32_Ehdr *h = MmAllocateKernelHeap(sizeof(struct Elf32_Ehdr));
    if(NULL == h)
    {
        IoCloseKernelFile(f);
        return OUT_OF_RESOURCES;
    }

    uint64_t bytesRead;
    if(OK != (ret = IoReadKernelFileSync(f, h, sizeof(*h), 0, &bytesRead)))
    {
        IoCloseKernelFile(f);
        MmFreeKernelHeap(h);
        return ret;
    }
    
    if(bytesRead != sizeof(*h))
    {
        IoCloseKernelFile(f);
        MmFreeKernelHeap(h);
        return EXEC_ELF_BROKEN;
    }

    if(OK != (ret = ExVerifyElf32Header(h))) //verify header
    {
        IoCloseKernelFile(f);
        MmFreeKernelHeap(h);
        return EXEC_ELF_BROKEN;
    }

    /**
     * Calculate required symbol table size first
    */

    struct Elf32_Sym *symbolTab = NULL;

    struct Elf32_Shdr *s = MmAllocateKernelHeap(sizeof(struct Elf32_Shdr)); //section header
    if(NULL == s)
    {
        ret = OUT_OF_RESOURCES;
        goto ExLoadKernelSymbolsFailed;
    }

    uint32_t symbolCount = 0;
    struct Elf32_Shdr *stringTabHdr = NULL;
    for(uint16_t i = 0; i < h->e_shnum; i++) //loop for all sections
	{
        if(OK != (ret = IoReadKernelFileSync(f, s, sizeof(*s), (uintptr_t)ExGetElf32SectionHeader(h, i) - (uintptr_t)h, &bytesRead)))
            goto ExLoadKernelSymbolsFailed;
        
        if(bytesRead != sizeof(*s))
        {
            ret = EXEC_ELF_BROKEN;
            goto ExLoadKernelSymbolsFailed;
        }

		if(SHT_SYMTAB == s->sh_type) //symbol table header
		{
			symbolTab = MmAllocateKernelHeap(sizeof(struct Elf32_Sym) * s->sh_size);
            if(NULL == symbolTab)
            {
                ret = OUT_OF_RESOURCES;
                goto ExLoadKernelSymbolsFailed;
            }
            if(OK != (ret = IoReadKernelFileSync(f, symbolTab, s->sh_size, s->sh_offset, &bytesRead)))
                goto ExLoadKernelSymbolsFailed;

            if(bytesRead != s->sh_size)
            {
                ret = EXEC_ELF_BROKEN;
                goto ExLoadKernelSymbolsFailed;
            }
            
            for(uint32_t i = 0; i < (s->sh_size / s->sh_entsize); i++) //loop for symbol entries
            {
                //skip hidden symbols
				if(STV_DEFAULT != ELF32_ST_VISIBILITY(symbolTab[i].st_other))
					continue;
                //check for symbol type
		        if(ELF32_ST_TYPE(symbolTab[i].st_info) == STT_FUNC || ELF32_ST_TYPE(symbolTab[i].st_info) == STT_OBJECT)
                {
                    //FIXME: awful assumption that all symbols have strings in the same string table
                    if(NULL == stringTabHdr)
                        stringTabHdr = ExGetElf32SectionHeader(h, s->sh_link); //get string table header
                    symbolCount++; //increment symbol count
                }
            }
            MmFreeKernelHeap(symbolTab);
		}
	}

    /**
     * Once the symbol count is known, allocate memory
    */
	kernelSymbolTable = MmAllocateKernelHeap(symbolCount * sizeof(struct KernelSymbol)); //allocate
    if(NULL == kernelSymbolTable) //check if allocation successful
    {
        ret = OUT_OF_RESOURCES;
        goto ExLoadKernelSymbolsFailed;
    }

    if(OK != (ret = IoReadKernelFileSync(f, s, sizeof(*s), (uintptr_t)stringTabHdr - (uintptr_t)h, &bytesRead)))
        goto ExLoadKernelSymbolsFailed;
    
    if(bytesRead != sizeof(*s))
    {
        ret = EXEC_ELF_BROKEN;
        goto ExLoadKernelSymbolsFailed;
    }
    
    kernelSymbolStrings = MmAllocateKernelHeap(s->sh_size);
    if(NULL == kernelSymbolStrings)
    {
        ret = OUT_OF_RESOURCES;
        goto ExLoadKernelSymbolsFailed;
    }

    if(OK != (ret = IoReadKernelFileSync(f, kernelSymbolStrings, s->sh_size, s->sh_offset, &bytesRead)))
        goto ExLoadKernelSymbolsFailed;
    
    if(bytesRead != s->sh_size)
    {
        ret = EXEC_ELF_BROKEN;
        goto ExLoadKernelSymbolsFailed;
    }

    for(uint16_t i = 0; i < h->e_shnum; i++) //loop for all sections
	{
        if(OK != (ret = IoReadKernelFileSync(f, s, sizeof(*s), (uintptr_t)ExGetElf32SectionHeader(h, i) - (uintptr_t)h, &bytesRead)))
            goto ExLoadKernelSymbolsFailed;
        
        if(bytesRead != sizeof(*s))
        {
            ret = EXEC_ELF_BROKEN;
            goto ExLoadKernelSymbolsFailed;
        }

		if(SHT_SYMTAB == s->sh_type) //symbol table header
		{
			symbolTab = MmAllocateKernelHeap(sizeof(struct Elf32_Sym) * s->sh_size);
            if(NULL == symbolTab)
            {
                ret = OUT_OF_RESOURCES;
                goto ExLoadKernelSymbolsFailed;
            }
            if(OK != (ret = IoReadKernelFileSync(f, symbolTab, s->sh_size, s->sh_offset, &bytesRead)))
                goto ExLoadKernelSymbolsFailed;

            if(bytesRead != s->sh_size)
            {
                ret = EXEC_ELF_BROKEN;
                goto ExLoadKernelSymbolsFailed;
            }
            for(uint32_t i = 0; i < (s->sh_size / s->sh_entsize); i++) //loop for symbol entries
            {
                //skip hidden symbols
				if(STV_DEFAULT != ELF32_ST_VISIBILITY(symbolTab[i].st_other))
					continue;
                //check for symbol type
		        if(ELF32_ST_TYPE(symbolTab[i].st_info) == STT_FUNC || ELF32_ST_TYPE(symbolTab[i].st_info) == STT_OBJECT)
                {
                    kernelSymbolTable[kernelSymbolCount].value = symbolTab[i].st_value; //store symbol value
                    kernelSymbolTable[kernelSymbolCount].name = &kernelSymbolStrings[symbolTab[i].st_name];
			        kernelSymbolCount++;
                }
            }
            MmFreeKernelHeap(symbolTab);
		}
	}

    ExLoadKernelSymbolsFailed:
    if(OK != ret)
        kernelSymbolCount = 0;
    MmFreeKernelHeap(h);
    MmFreeKernelHeap(s);
    MmFreeKernelHeap(symbolTab);
    IoCloseKernelFile(f);
    return ret;
}

uintptr_t ExGetKernelSymbol(const char *name)
{
    //look for kernel symbol
    for(uint32_t i = 0; i < kernelSymbolCount; i++)
    {
        if(0 == CmStrcmp(name, kernelSymbolTable[i].name))
            return kernelSymbolTable[i].value;
    }
    return 0;
}
