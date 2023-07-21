#include "exec.h"

#include "mm/heap.h"
#include "common.h"
#include "mm/valloc.h"
#include "mm/mm.h"
#include "elf.h"


//#define EXEC_PROVIDE_KERNEL_OBJECTS //provide non-function kernel objects to programs linked against kernel

struct KernelSymbol
{
	uintptr_t value;
	char *name;
};

struct KernelSymbol *kernelSymbolTable = NULL;
uint32_t kernelSymbolCount = 0;


STATUS Ex_loadKernelSymbols(uintptr_t kernelImage)
{
    STATUS ret = OK;

    struct Elf32_Ehdr *h = (struct Elf32_Ehdr*)kernelImage; //get ELF header

    ret = Ex_verifyElf32Header(h); //verify header
    if(OK != ret)
        return ret;

    /**
     * Calculate required symbol table size first
    */
    uint32_t symbolCount = 0;
    struct Elf32_Shdr *s; //section header
    for(uint16_t i = 0; i < h->e_shnum; i++) //loop for all sections
	{
		s = Ex_getElf32SectionHeader(h, i); //get section header

		if(SHT_SYMTAB == s->sh_type) //symbol table header
		{
			struct Elf32_Sym *symbol = (struct Elf32_Sym*)((uintptr_t)h + s->sh_offset); //get symbol structure
            for(uint32_t i = 0; i < (s->sh_size / s->sh_entsize); i++) //loop for symbol entries
            {
                //check for symbol type
#ifdef EXEC_PROVIDE_KERNEL_OBJECTS
		        if(ELF32_ST_TYPE(symbol[i].st_info) == STT_FUNC || ELF32_ST_TYPE(symbol[i].st_info) == STT_OBJECT)
#else
                if(ELF32_ST_TYPE(symbol[i].st_info) == STT_FUNC)
#endif
                {
                    symbolCount++; //increment symbol count
                }
            }
		}
	}

    /**
     * When symbol count is known, allocate memory and get symbols
    */
	kernelSymbolTable = MmAllocateKernelHeap(symbolCount * sizeof(struct KernelSymbol)); //allocate
    if(NULL == kernelSymbolTable) //check if allocation successful
        return EXEC_MALLOC_FAILED;

    for(uint16_t i = 0; i < h->e_shnum; i++) //loop for all sections
	{
		s = Ex_getElf32SectionHeader(h, i); //get section header

		if(SHT_SYMTAB == s->sh_type) //symbol table header
		{
			struct Elf32_Sym *symbol = (struct Elf32_Sym*)((uintptr_t)h + s->sh_offset); //get symbol structure
            for(uint32_t i = 0; i < (s->sh_size / s->sh_entsize); i++) //loop for symbol entries
            {
                //check for symbol type
#ifdef EXEC_PROVIDE_KERNEL_OBJECTS
		        if(ELF32_ST_TYPE(symbol[i].st_info) == STT_FUNC || ELF32_ST_TYPE(symbol[i].st_info) == STT_OBJECT)
#else
                if(ELF32_ST_TYPE(symbol[i].st_info) == STT_FUNC)
#endif
                {
                    struct Elf32_Shdr *strTab = Ex_getElf32SectionHeader(h, s->sh_link); //get string table header
                    char *name = (char*)h + strTab->sh_offset + symbol[i].st_name; //get symbol name in string table

                    kernelSymbolTable[kernelSymbolCount].value = symbol[i].st_value; //store symbol value
                    if(NULL == (kernelSymbolTable[kernelSymbolCount].name = MmAllocateKernelHeap(CmStrlen(name) + 1))) //allocate memory for symbol name
                        return EXEC_MALLOC_FAILED;

			        CmStrcpy(kernelSymbolTable[kernelSymbolCount].name, name); //copy symbol name
			        kernelSymbolCount++;
                }
            }
		}
	}

    return OK;
}

uintptr_t Ex_getKernelSymbol(const char *name)
{
    //look for kernel symbol
    for(uint32_t i = 0; i < kernelSymbolCount; i++)
    {
        if(0 == CmStrcmp(name, kernelSymbolTable[i].name))
            return kernelSymbolTable[i].value;
    }
    return 0;
}

