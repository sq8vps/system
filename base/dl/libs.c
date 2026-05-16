#include "libs.h"
#include "elf.h"
#include <errno.h>


static int DlLoadSharedLibrary(const char *name, const char **envp, struct Elf32_Ehdr **lh)
{
    
}

int DlLoadLibs(const struct Elf32_Ehdr *h, const char **envp)
{
    struct Elf32_Phdr *phdr = nullptr; //program data headers
    struct Elf32_Dyn *dyn = nullptr; //dynamic entries
    const char *strTab = nullptr; //string table

    if(DlVerifyElf32Header(h) < 0)
        return -ENOEXEC;

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
                return -ENOEXEC;
            
            for(size_t i = 0; i < (phdr->p_memsz / sizeof(struct Elf32_Dyn)); i++)
            {
                if(DT_NEEDED == dyn[i].d_tag)
                {
                    struct Elf32_Ehdr *lh = nullptr;
                    int result = 0;
                    result = DlLoadSharedLibrary(&strTab[dyn[i].d_un.d_val], envp, &lh);
                    if(result < 0)
                        return result;

                    
                }
            }
        }
        phdr = (struct Elf32_Phdr*)((uintptr_t)phdr + h->e_phentsize);
    }

    return -ENOERR;
}