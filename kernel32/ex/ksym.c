#include "ksym.h"
#include "elf.h"
#include "multiboot.h"
#include "mm/dynmap.h"
#include "mm/heap.h"
#include "ke/core/panic.h"
#include "rtl/string.h"

struct
{
    const char *name;
    uintptr_t value;
}
static *ExKernelSymbolTable;
static uint32_t ExKernelSymbolCount = 0;

STATUS ExLoadKernelSymbols(struct Multiboot2InfoHeader *mb2h)
{
    const struct Multiboot2InfoTag *tag = Multiboot2FindTag(mb2h, NULL, MB2_ELF_SYMBOLS);
    if(NULL != tag)
    {
        const struct Multiboot2ElfSymbolsTag *elf = (const struct Multiboot2ElfSymbolsTag*)tag;
        uint16_t count = elf->num; //get section header count
        const struct Elf32_Shdr *s = (const struct Elf32_Shdr*)(elf + 1);
        while(0 != count)
        {
            if(SHT_SYMTAB == s->sh_type)
            {
                const struct Elf32_Sym *symtab = MmMapDynamicMemory(s->sh_addr, s->sh_size, MM_FLAG_READ_ONLY);
                if(NULL == symtab)
                    FAIL_BOOT("cannot map kernel symbol table");
                
                //first pass - calculate number of usable symbols
                for(uint32_t i = 0; i < (s->sh_size / s->sh_entsize); i++)
                {
                    //skip hidden symbols
                    if(STV_DEFAULT != ELF32_ST_VISIBILITY(symtab[i].st_other))
                        continue;
                    //check for symbol type
                    if((STT_FUNC == ELF32_ST_TYPE(symtab[i].st_info)) || (STT_OBJECT == ELF32_ST_TYPE(symtab[i].st_info)))
                    {
                        ExKernelSymbolCount++;
                    }
                }

                ExKernelSymbolTable = MmAllocateKernelHeap(ExKernelSymbolCount * sizeof(*ExKernelSymbolTable));
                if(NULL == ExKernelSymbolTable)
                    FAIL_BOOT("cannot allocate memory for kernel symbol table");

                //sh_link should contain the associated string table index
                if(s->sh_link > elf->num)
                    FAIL_BOOT("kernel string table index out of bounds");
                struct Elf32_Shdr *str = (struct Elf32_Shdr*)((uintptr_t)(elf + 1) + (s->sh_link * elf->entsize));
                if(SHT_STRTAB != str->sh_type)
                    FAIL_BOOT("kernel symbol table is broken");
                //this must remain allocated
                const char *const strtab = MmMapDynamicMemory(str->sh_addr, str->sh_size, MM_FLAG_READ_ONLY);
                if(NULL == strtab)
                    FAIL_BOOT("cannot map kernel string table");
                
                //second pass - store symbols
                ExKernelSymbolCount = 0;
                for(uint32_t i = 0; i < (s->sh_size / s->sh_entsize); i++)
                {
                    //skip hidden symbols
                    if(STV_DEFAULT != ELF32_ST_VISIBILITY(symtab[i].st_other))
                        continue;
                    //check for symbol type
                    if((STT_FUNC == ELF32_ST_TYPE(symtab[i].st_info)) || (STT_OBJECT == ELF32_ST_TYPE(symtab[i].st_info)))
                    {
                        ExKernelSymbolTable[ExKernelSymbolCount].value = symtab[i].st_value;
                        ExKernelSymbolTable[ExKernelSymbolCount].name = &(strtab[symtab[i].st_name]);
                        ExKernelSymbolCount++;
                    }
                }

                MmUnmapDynamicMemory(symtab);
                return OK;
            }
            s = (const struct Elf32_Shdr*)((uintptr_t)s + elf->entsize);
            --count;
        }
    }

    FAIL_BOOT("kernel symbol table missing");

    return EXEC_ELF_BROKEN;
}

uintptr_t ExGetKernelSymbol(const char *name)
{
    if(unlikely(NULL == ExKernelSymbolTable))
        return 0;
    
    for(uint32_t i = 0; i < ExKernelSymbolCount; i++)
    {
        if(0 == RtlStrcmp(name, ExKernelSymbolTable[i].name))
            return ExKernelSymbolTable[i].value;
    }
    return 0;
}
