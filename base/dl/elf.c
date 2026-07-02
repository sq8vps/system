#include "elf.h"
#include "libs.h"
#include <string.h>

STATUS DlVerifyElf32Header(const struct Elf32_Ehdr *h)
{
	if(h->ei_mag[0] != ELFMAG0 || h->ei_mag[1] != ELFMAG1 || h->ei_mag[2] != ELFMAG2 || h->ei_mag[3] != ELFMAG3) //check for magic number
		return BAD_TYPE;

	if(h->ei_class != ELFCLASS32)
		return BAD_TYPE;

	if(h->ei_data != ELFDATA2LSB)
		return BAD_TYPE;

	if(h->e_machine != EM_386)
		return BAD_TYPE;

	if(h->ei_version != EV_CURRENT)
		return BAD_TYPE;

	if(h->e_version != EV_CURRENT)
		return BAD_TYPE;

	return OK;
}

static uint32_t DlElf32Hash(const char *name)
{
    const unsigned char *n = (const unsigned char*)name;
    uint32_t h = 0, g;
    while(*n)
    {
        h = (h << 4) + *n++;
        g = h & 0xf0000000;
        if(0 != g)
            h ^= g >> 24;
        h &= ~g;
    }
    return h;
}

static STATUS DlGetElf32SymbolValueByName(const char *name, uint32_t *value)
{
	struct DlLibrary *lib = DlState.loaded.list;

    while(nullptr != lib)
	{
        if(nullptr != lib->hashTab)
        {
            uint32_t hash = DlElf32Hash(name);
            size_t index = lib->hashTab->data[hash % lib->hashTab->nbucket];

            while(1)
            {
                struct Elf32_Sym *sym = &lib->dynSym[index];
                if(0 == strcmp(name, &lib->dynStr[sym->st_name]))
                {
                    if(ELF32_ST_TYPE(sym->st_info) == STT_FUNC || ELF32_ST_TYPE(sym->st_info) == STT_OBJECT)
                    {
                        if(SHN_UNDEF == sym->st_shndx)
                        {
                            break;
                        }
                        else if(SHN_ABS == sym->st_shndx) 
                        {
                            *value = sym->st_value;
                            return OK;
                        }
                        else
                        {
                            *value = ((uintptr_t)lib->h + sym->st_value);
                            return OK;
                        }
                    }
                }
                index = lib->hashTab->data[lib->hashTab->nbucket + index];
                if(STN_UNDEF == index)
                    break;
            }
        }
        lib = lib->next;
	}

    return UNDEFINED_SYMBOL;
}

static STATUS DlGetElf32SymbolValue(const struct Elf32_Ehdr *h, struct Elf32_Sym *symbol, const char *strTab, uint32_t *value)
{
    STATUS status = OK;
    uintptr_t base = (uintptr_t)h;
	if(SHN_UNDEF == symbol->st_shndx) //external symbol
	{
		const char *name = strTab + symbol->st_name;
        uint32_t v = 0;

		status = DlGetElf32SymbolValueByName(name, &v);

		if(UNDEFINED_SYMBOL == status)
		{
			if(ELF32_ST_BIND(symbol->st_info) & STB_WEAK) //but we have a weak value
			{
				*value = base + symbol->st_value;
				return OK; 
			}
			else
			{
				return UNDEFINED_SYMBOL;
			}
		}
		else
		{
			*value = v;
		}
	}
	else if(SHN_ABS == symbol->st_shndx) //absolute value
	{
		*value = symbol->st_value;
	}
	else //normal symbol - virtual address in executables and shared objects
	{
		*value = base + symbol->st_value;
	}

	return OK;
}

static STATUS DlRelocateElf32Symbol(const struct Elf32_Ehdr *h, const struct Elf32_Rel *rel, bool isRela, struct Elf32_Sym *symTab, const char *strTab)
{
    STATUS status = 0;
    uintptr_t base = (uintptr_t)h;
	uint32_t value = 0; //symbol value should default to 0
    uint32_t *target = (uint32_t*)(rel->r_offset + base); //target symbol location
    int32_t addend = isRela ? ((const struct Elf32_Rela*)(rel))->r_addend : 0;

	//is symbol defined?
	if(STN_UNDEF != ELF32_R_SYM(rel->r_info))
	{
		status = DlGetElf32SymbolValue(h, &symTab[ELF32_R_SYM(rel->r_info)], strTab, &value);
		if(OK != status)
			return status;
	}
	//if symbol is not defined, default to 0
	switch(ELF32_R_TYPE(rel->r_info))
	{
		case R_386_NONE:
			break;
        case R_386_GLOB_DAT: //S
        case R_386_JMP_SLOT: //S
            *target = value;
            break;
        case R_386_32: //S+A
            *target = value + (isRela ? addend : (int32_t)*target);
            break;
        case R_386_RELATIVE: //B+A
            *target = base + (isRela ? addend : (int32_t)*target);
            break;
		default:
			return NOT_SUPPORTED;
			break;
	}
	return OK;
}

STATUS DlPerformRelocations(struct Elf32_Ehdr *h)
{
	STATUS status = OK;
    uintptr_t base = (uintptr_t)h; //file base
    struct Elf32_Phdr *phdr = nullptr; //program data headers
    struct Elf32_Dyn *dyn = nullptr; //dynamic entries
    struct Elf32_Rel *pltRel = nullptr; //PLT-associated relocation entries
    size_t pltRelCount = 0; //number of PLT-related relocation entries
    bool pltAddend = false; //are PLT relocations with addends?
    struct Elf32_Rel *rel = nullptr; //dynamic relocations
    size_t relCount = 0; //number of dynamic relocation entries
    struct Elf32_Rela *rela = nullptr; //dynamic relocations with addends
    size_t relaCount = 0; //number of entries of dynamic relocations with addends 
    struct Elf32_Sym *sym = nullptr; //symbol table
    const char *str = nullptr; //string table

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
                    case DT_STRTAB:
                        str = (const char *)(dyn[i].d_un.d_ptr + base);
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
	{
        goto leave;
	}

    //perform PLT-associated relocations
    for(size_t i = 0; i < pltRelCount; i++)
    {
        status = DlRelocateElf32Symbol(h, &pltRel[i], pltAddend, sym, str);
        if(OK != status)
            return status;
    }

    //perform dynamic relocations with implicit addends
    for(size_t i = 0; i < relCount; i++)
    {
        status = DlRelocateElf32Symbol(h, &rel[i], false, sym, str);
        if(OK != status)
            return status;
    }

    //perform dynamic relocations with explicit addends
    for(size_t i = 0; i < relaCount; i++)
    {
        status = DlRelocateElf32Symbol(h, (const struct Elf32_Rel*)&rela[i], true, sym, str);
        if(OK != status)
            return status;
    }

leave:
	return status;
}