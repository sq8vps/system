#include "elf.h"
#include "io/log/syslog.h"
#include "rtl/string.h"

struct Elf32_Shdr* ExGetElf32SectionHeader(struct Elf32_Ehdr *h, uint16_t n)
{
	return &((struct Elf32_Shdr*)((uintptr_t)h + h->e_shoff))[n];
}

STATUS ExGetElf32SymbolValueByName(struct Elf32_Ehdr *h, char *name, uintptr_t *symbolValue)
{
    STATUS ret = OK;

    ret = ExVerifyElf32Header(h); //verify header
    if(OK != ret)
	{
        return ret;
	}

	struct Elf32_Shdr *s;

    for(uint16_t i = 0; i < h->e_shnum; i++) //loop for all sections
	{
		s = ExGetElf32SectionHeader(h, i); //get section header

		if(SHT_SYMTAB == s->sh_type) //symbol table header
		{
			struct Elf32_Sym *symbol = (struct Elf32_Sym*)((uintptr_t)h + s->sh_offset); //get symbol structure
            for(uint32_t i = 0; i < (s->sh_size / s->sh_entsize); i++) //loop for symbol entries
            {
                //check for symbol type
		        if(ELF32_ST_TYPE(symbol[i].st_info) == STT_FUNC || ELF32_ST_TYPE(symbol[i].st_info) == STT_OBJECT)
                {
                    struct Elf32_Shdr *strTab = ExGetElf32SectionHeader(h, s->sh_link); //get string table header
                    if(0 != RtlStrcmp((char*)h + strTab->sh_offset + symbol[i].st_name, name)) //compare names
						continue;

					if(SHN_UNDEF == symbol[i].st_shndx) //symbol is undefined
					{
						return UNDEFINED_SYMBOL;
					}
					else if(SHN_ABS == symbol[i].st_shndx) //absolute symbol not affected by relocation
					{
						//st_value contains this absolute symbol value
						*symbolValue = symbol[i].st_value;
						return OK;
					}
					else //symbol defined in the ELF image
					{
						//st_shndx is the section header index and is used to obatin sh_offset which is a section offset
						//st_value is the offset inside given section
						*symbolValue = (uintptr_t)h + ExGetElf32SectionHeader(h, symbol[i].st_shndx)->sh_offset + symbol[i].st_value;
						return OK;
					}
                }
            }
		}
	}

    return UNDEFINED_SYMBOL;
}

STATUS ExGetElf32SymbolValue(struct Elf32_Ehdr *h, uint16_t table, uint32_t index, uintptr_t *symbolValue, ExElfResolver_t resolver)
{
	if((SHN_UNDEF == table) || (SHN_UNDEF == index))
	{
		*symbolValue = 0;
		return OK;
	}

	struct Elf32_Shdr *symTabHdr = ExGetElf32SectionHeader(h, table); //get appropriate symbol table header
	uint32_t symTabEntries = symTabHdr->sh_size / symTabHdr->sh_entsize; //calculate number of entries
	if(index >= symTabEntries) //index exceeding table boundaries
	{
		return CORRUPTED;
	}

	//get symbol structure from symbol table
	struct Elf32_Sym *symbol = &((struct Elf32_Sym*)((uintptr_t)h + symTabHdr->sh_offset))[index];

	if(SHN_UNDEF == symbol->st_shndx) //external symbol, external linking required
	{
		struct Elf32_Shdr *stringTabHdr = ExGetElf32SectionHeader(h, symTabHdr->sh_link); //get string table header
		const char *name = (const char*)h + stringTabHdr->sh_offset + symbol->st_name; //get string for this symbol

		//perform linking with external symbol
		uint32_t value = (nullptr != resolver) ? resolver(name) : 0;

		if(0 == value) //symbol not found
		{
			if(ELF32_ST_BIND(symbol->st_info) & STB_WEAK) //this symbol was declared as weak and no definition was found
			{
				*symbolValue = value; //return weak value
				return OK; 
			}
			else //no definition found and this symbol is not weak: failure
			{
				LOG(SYSLOG_ERROR, "Unknown symbol: %s", name);
				return UNDEFINED_SYMBOL;
			}
		}
		else //external symbol value found
		{
			*symbolValue = value;
			return OK;
		}
	}
	else if(SHN_ABS == symbol->st_shndx) //absolute symbol not affected by relocation
	{
		//st_value contains this absolute symbol value
		*symbolValue = symbol->st_value;
		return OK;
	}
	else //symbol defined in the ELF image
	{
		//st_shndx is the section header index and is used to obatin sh_offset which is a section offset
		//st_value is the offset inside given section
		*symbolValue = (uintptr_t)h + ExGetElf32SectionHeader(h, symbol->st_shndx)->sh_offset + symbol->st_value;
		return OK;
	}

	return UNDEFINED_SYMBOL;
}

STATUS ExRelocateElf32Symbol(struct Elf32_Ehdr *h, struct Elf32_Shdr *relSectionHdr, struct Elf32_Rel *relEntry, ExElfResolver_t resolver)
{
	 //get target section header, i.e. the section where relocation will be applied
	struct Elf32_Shdr *targetSectionHdr = ExGetElf32SectionHeader(h, relSectionHdr->sh_info);

	//get pointer to the point that needs to be relocated
	uint32_t *point = (uint32_t*)((uintptr_t)h + targetSectionHdr->sh_offset + relEntry->r_offset);

	uint32_t value = 0; //symbol value should default to 0

	//check if symbol to be relocated is defined
	if(SHN_UNDEF != ELF32_R_SYM(relEntry->r_info))
	{
		//get it's value
		STATUS ret = ExGetElf32SymbolValue(h, relSectionHdr->sh_link, ELF32_R_SYM(relEntry->r_info), &value, resolver);
		if(OK != ret)
			return ret; //return on failure
	}
	//if not, use default 0 value

	switch(ELF32_R_TYPE(relEntry->r_info)) //apply appropriate relocation
	{
		case R_386_NONE: //no reallocation
			break;
		case R_386_32: //reallocation without section offset (symbol + addend)
			*point += value;
			break;
		case R_386_PC32: //reallocation with section offset (symbol + addend - offset)
			*point = *point + value - (uintptr_t)point;
			break;
		default:
			return NOT_SUPPORTED;
			break;
	}
	return OK;
}

STATUS ExPerformElf32Relocation(struct Elf32_Ehdr *h, ExElfResolver_t resolver)
{
	STATUS ret = OK;
	
	struct Elf32_Shdr *sectionHdr;

	for(uint16_t i = 0; i < h->e_shnum; i++) //iterate thorugh all sections
	{
		sectionHdr = ExGetElf32SectionHeader(h, i); //get header

		if(SHT_REL != sectionHdr->sh_type) //skip sections other than relocation
			continue;

		struct Elf32_Rel *entries = ((struct Elf32_Rel*)((uintptr_t)h + sectionHdr->sh_offset)); //get entry table
		for(uint32_t k = 0; k < (sectionHdr->sh_size / sectionHdr->sh_entsize); k++) //loop for every entry
		{
			ret = ExRelocateElf32Symbol(h, sectionHdr, &entries[k], resolver); //relocate symbol
			if(OK != ret)
				return ret;
		}
	}
	return OK;
}



STATUS ExVerifyElf32Header(struct Elf32_Ehdr *h)
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
		return CORRUPTED;

	if(h->e_version != EV_CURRENT)
		return CORRUPTED;

	return OK;
}