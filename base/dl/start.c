#include <ex/load.h>
#include <ke/task/task.h>
#include <stddef.h>
#include <limits.h>
#include "elf.h"
#include <sys/nabla.h>
#include <stdlib.h>

int main(int argc, char **argv, char **envp, struct ExProgramData *progData);

/**
 * @brief Perform self relocation and call main()
 */
[[noreturn]] void _start(int argc, char **argv, char **envp, struct ExProgramData *progData)
{
    struct ExProgramData *p = progData;
    uintptr_t base = 0; //linker base
    struct Elf32_Ehdr *h = nullptr; //ELF header
    struct Elf32_Phdr *phdr = nullptr; //program data headers
    struct Elf32_Dyn *dyn = nullptr; //dynamic entries
    struct Elf32_Rel *pltRel = nullptr; //PLT-associated relocation entries
    size_t pltRelCount = 0; //number of PLT-related relocation entries
    bool pltAddend = false; //are PLT relocations with addends?
    struct Elf32_Rel *rel = nullptr; //dynamic relocations
    size_t relCount = 0; //number of dynamic relocation entries
    struct Elf32_Rela *rela = nullptr; //dynamic relocations with addedts
    size_t relaCount = 0; //number of entries of dynamic relocations with addends 
    struct Elf32_Sym *sym = nullptr; //symbol table
    while(PROGDATA_END != p->type)
    {
        if(PROGDATA_LINKER_BASE == p->type)
        {
            base = (uintptr_t)p->value.p;
            h = p->value.p;
            break;
        }
        ++p;
    }

    if((h->ei_mag[0] != ELFMAG0) || (h->ei_mag[1] != ELFMAG1) || (h->ei_mag[2] != ELFMAG2) || (h->ei_mag[3] != ELFMAG3)
        || (h->ei_class != ELFCLASS32)
        || (h->ei_data != ELFDATA2LSB)
        || (h->e_machine != EM_386)
        || (h->ei_version != EV_CURRENT)
        || (h->e_version != EV_CURRENT))
    {
        goto fail;
    }

    phdr = (struct Elf32_Phdr*)(h->e_phoff + base);
    for(size_t i = 0; i < h->e_phnum; i++)
    {
        if(PT_DYNAMIC == phdr->p_type)
        {
            size_t pltRelSize = 0;
            dyn = (struct Elf32_Dyn*)(phdr->p_vaddr + base);
            for(size_t i = 0; i < (phdr->p_memsz / sizeof(struct Elf32_Dyn)); i++)
            {
                switch(dyn[i].d_tag)
                {
                    case DT_PLTRELSZ:
                        pltRelSize = dyn[i].d_un.d_val;
                        break;
                    case DT_SYMTAB:
                        sym = (struct Elf32_Sym*)(dyn[i].d_un.d_ptr + base);
                        break;
                    case DT_PLTREL:
                        if(DT_RELA == dyn[i].d_un.d_val)
                            pltAddend = true;
                        break;
                    case DT_JMPREL:
                        pltRel = (struct Elf32_Rel*)(dyn[i].d_un.d_ptr + base);
                        break;
                    case DT_RELA:
                        rela = (struct Elf32_Rela*)(dyn[i].d_un.d_ptr + base);
                        break;
                    case DT_REL:
                        rel = (struct Elf32_Rel*)(dyn[i].d_un.d_ptr + base);
                        break;
                    case DT_RELASZ:
                        relaCount = dyn[i].d_un.d_val / sizeof(struct Elf32_Rela);
                        break;
                    case DT_RELSZ:
                        relCount = dyn[i].d_un.d_val / sizeof(struct Elf32_Rel);
                        break;
                    default:
                        break;
                }
            }
            pltRelCount = pltRelSize / (pltAddend ? sizeof(struct Elf32_Rela) : sizeof(struct Elf32_Rel));
            break;
        }
        phdr = (struct Elf32_Phdr*)((uintptr_t)phdr + h->e_phentsize);
    }

    if(nullptr == dyn)
        goto fail;

    //perform PLT-associated relocations
    for(size_t i = 0; i < pltRelCount; i++)
    {
        // int32_t add = 0;
        // if(pltAddend)
        //     add = ((struct Elf32_Rela*)(&pltRel[i]))->r_addend;

        switch(ELF32_R_TYPE(pltRel[i].r_info))
        {   
            case R_386_JMP_SLOT:
                *((uint32_t*)(pltRel[i].r_offset + base)) = sym[ELF32_R_SYM(pltRel[i].r_info)].st_value + base;
                break;
            default:
                goto fail;
        }
    }

    //perform dynamic relocations without addends
    for(size_t i = 0; i < relCount; i++)
    {
        switch(ELF32_R_TYPE(rel[i].r_info))
        {   
            case R_386_GLOB_DAT:
                *((uint32_t*)(rel[i].r_offset + base)) = sym[ELF32_R_SYM(rel[i].r_info)].st_value + base;
                break;
            case R_386_32: //S+A
                *((uint32_t*)(rel[i].r_offset + base)) += sym[ELF32_R_SYM(rel[i].r_info)].st_value + base;
                break;
            default:
                goto fail;
        }
    }

    //perform dynamic relocations with addends
    for(size_t i = 0; i < relaCount; i++)
    {
        switch(ELF32_R_TYPE(rela[i].r_info))
        {   
            case R_386_GLOB_DAT:
                *((uint32_t*)(rela[i].r_offset + base)) = sym[ELF32_R_SYM(rela[i].r_info)].st_value + base;
                break;
            case R_386_32: //S+A
                *((uint32_t*)(rela[i].r_offset + base)) = sym[ELF32_R_SYM(rela[i].r_info)].st_value + base + rela[i].r_addend;
                break;
            default:
                goto fail;
        }
    }

    int result = __nabla_init_libc(argc, argv, envp, progData);
    if(result < 0)
        ApiExitTask(result);
    
    exit(main(argc, argv, envp, progData));

    fail:
    //TODO: how to handle this fail?
    *((volatile int*)0x10) = 12345;

    while(1)
        ;
}