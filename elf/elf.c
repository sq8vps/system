#include "elf.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define ELF_KERNEL_PROVIDE_OBJECTS //uncomment to provide kernel non-function object symbols (basically variables)

enum Elf32_e_type
{
	ET_NONE = 0,
	ET_REL = 1,
	ET_EXEC = 2,
	ET_DYN = 3,
	ET_CORE = 4
};

enum Elf32_e_machine
{
	EM_NONE = 0,
	EM_M32 = 1,
	EM_SPARC = 2,
	EM_386 = 3,
	EM_68K = 4,
	EM_88K = 5,
	EM860 = 7,
	EM_MIPS = 8,
	EM_MIPS_RS4_BE = 10,
};

enum Elf32_e_version
{
	EV_NONE = 0,
	EV_CURRENT = 1,
};

enum Elf32_ei_mag
{
	ELFMAG0 = 0x7f,
	ELFMAG1 = 'E',
	ELFMAG2 = 'L',
	ELFMAG3 = 'F',
};

enum Elf32_ei_class
{
	ELFCLASSNONE = 0,
	ELFCLASS32 = 1,
	ELFCLASS64 = 2,
};

enum Elf32_ei_data
{
	ELFDATANONE = 0,
	ELFDATA2LSB = 1,
	ELFDATA2MSB = 2,
};

struct Elf32_Ehdr
{
        uint8_t ei_mag[4];
        uint8_t ei_class;
        uint8_t ei_data;
        uint8_t ei_version;
        uint8_t ei_abi;
        uint64_t unused;
        uint16_t e_type;
        uint16_t e_machine;
        uint32_t e_version;
        uint32_t e_entry;
        uint32_t e_phoff;
        uint32_t e_shoff;
        uint32_t e_flags;
        uint16_t e_ehsize;
        uint16_t e_phentsize;
        uint16_t e_phnum;
        uint16_t e_shentsize;
        uint16_t e_shnum;
        uint16_t e_shstrndx;
} __attribute__ ((packed));

enum Elf32_p_type
{
	PT_NULL = 0,
	PT_LOAD = 1,
	PT_DYNAMIC = 2,
	PT_INTERP = 3,
	PT_NOTE = 4,
	PT_SHLIB = 5,
	PT_PHDR = 6,
	PT_LOPROC = 0x70000000,
	PT_HIPROC = 0x7fffffff,
};

struct Elf32_Phdr
{
	uint32_t p_type;
	uint32_t p_offset;
	uint32_t p_vaddr;
	uint32_t p_paddr;
	uint32_t p_filesz;
	uint32_t p_memsz;
	uint32_t p_flags;
	uint32_t p_align;
} __attribute__ ((packed));

enum Elf32_sh_type
{
	SHT_NULL = 0,
	SHT_PROGBITS = 1,
	SHT_SYMTAB = 2,
	SHT_STRTAB = 3,
	SHT_RELA = 4,
	SHT_HASH = 5,
	SHT_DYNAMIC = 6,
	SHT_NOTE = 7,
	SHT_NOBITS = 8,
	SHT_REL = 9,
	SHT_SHLIB = 10,
	SHT_DYNSYM = 11,
	SHT_LOPROC = 0x70000000,
	SHT_HIPROC = 0x7fffffff,
	SHT_LOUSER = 0x80000000,
	SHT_HIUSER = 0xffffffff,
};

enum Elf32_sh_flags
{
	SHF_WRITE = 0x1,
	SHF_ALLOC = 0x2,
	SHF_EXECINSTR = 0x4,
	SHF_MASKPROC = 0xf0000000,
};

#define SHN_UNDEF (0)
#define SHN_ABS (0xfff1)

struct Elf32_Shdr
{
	uint32_t sh_name;
	uint32_t sh_type;
	uint32_t sh_flags;
	uint32_t sh_addr;
	uint32_t sh_offset;
	uint32_t sh_size;
	uint32_t sh_link;
	uint32_t sh_info;
	uint32_t sh_addralign;
	uint32_t sh_entsize;
} __attribute__ ((packed));

struct Elf32_Sym
{
	uint32_t st_name;
	uint32_t st_value;
	uint32_t st_size;
	uint8_t st_info;
	uint8_t st_other;
	uint16_t st_shndx;
} __attribute__ ((packed));

//st_info field manipulation macros
#define ELF32_ST_BIND(i) ((i) >> 4)
#define ELF32_ST_TYPE(i) ((i) & 0xf)
#define ELF32_ST_INFO(b, t) (((b) << 4) + ((t) & 0xf))

#define STB_LOCAL 0
#define STB_GLOBAL 1
#define STB_WEAK 2
#define STB_LOPROC 13
#define STB_HIPROC 15

#define STT_NOTYPE 0
#define STT_OBJECT 1
#define STT_FUNC 2
#define STT_SECTION 3
#define STT_FILE 4
#define STT_LOPROC 13
#define STT_HIPROC 15

struct Elf32_Rel
{
	uint32_t r_offset;
	uint32_t r_info;
} __attribute__ ((packed));

struct Elf32_Rela
{
	uint32_t r_offset;
	uint32_t r_info;
	int32_t r_addend;
} __attribute__ ((packed));


#define ELF32_R_SYM(i)	((i) >> 8)
#define ELF32_R_TYPE(i)	((uint8_t)(i))
#define ELF32_R_INFO(s, t) (((s) << 8) + (uint8_t)(t))
 
enum Elf32_Rel_types {
	R_386_NONE = 0, //no relocation
	R_386_32 = 1, //symbol + addend
	R_386_PC32 = 2,  //symbol + addend - section offset
};



// kError_t elf_loadProgramSegments(struct Elf32_Ehdr *h, uint8_t *name)
// {
// 	struct Elf32_Phdr *p;
// 	for(uint16_t i = 0; i < h->e_phnum; i++)
// 	{
// 		p = (struct Elf32_Phdr *)(buf + i * h->e_phentsize); //get next program header entry

// 		if(p->p_type != PT_LOAD) //only PT_LOAD type
// 			continue;
// 		if(p->p_memsz == 0) //skip empty segments
// 			continue;
// 		if(p->p_filesz > p->p_memsz) //file size must not be bigger than memory size
// 			return EXEC_ELF_BROKEN;

// 		// ret = Mm_allocateEx(p->vAddr, p->memSize / MM_PAGE_SIZE + ((p->memSize % MM_PAGE_SIZE) ? 1 : 0), 0); //allocate page(s)
// 		// if(ret != OK)
// 		// 	return ret;

// 		// ret = Fat_readFile(&fatDisk, name, p->offset, p->fileSize, (uint8_t*)(p->vAddr)); //read segment
// 		// if(ret != OK)
// 		// 	return ret;

// 		// for(uint32_t i = p->fileSize; i < p->memSize; i++) //fill rest with zeros
// 		// 	*((uint8_t*)(p->vAddr + i)) = 0;
// 	}
// }

/**
 * @brief Get section header
 * @param h ELF file header
 * @param n Section index
 * @return Section header address
*/
static inline struct Elf32_Shdr *elf_getSectionHeader(struct Elf32_Ehdr *h, uint16_t n)
{
	return &((struct Elf32_Shdr*)((uintptr_t)h + h->e_shoff))[n];
}

struct KernelSymbol
{
	uint32_t value;
	char *name;
};

struct KernelSymbol *kernelSymbolTable = NULL;
uint32_t kernelSymbolCount = 0;

kError_t Elf_getKernelSymbolTable(const char *path)
{
	FILE *f = fopen(path, "rb");
	struct Elf32_Ehdr h;

	if(sizeof(h) != fread(&h, 1, sizeof(h), f)) //read header
	{
		return EXEC_ELF_BROKEN;
	}

	char *stringTable = NULL;
	struct Elf32_Sym *symbolTable = NULL;

	uint32_t rawSymbolCount = 0;

	struct Elf32_Shdr s; //section header
	
	for(uint16_t i = 0; i < h.e_shnum; i++) //loop for all sections
	{
		fseek(f, ((uintptr_t)elf_getSectionHeader(&h, i) - (uintptr_t)&h), SEEK_SET);
		if(sizeof(s) != fread(&s, 1, sizeof(s), f))
		{
			return EXEC_ELF_BROKEN;
		}

		if(SHT_SYMTAB == s.sh_type) //symbol table header
		{
			if(NULL == symbolTable)
			{
				symbolTable = malloc(s.sh_size);
				fseek(f, (uintptr_t)(s.sh_offset), SEEK_SET);
				if(s.sh_size != fread(symbolTable, 1, s.sh_size, f))
				{
					free(symbolTable);
					if(NULL != stringTable)
						free(stringTable);
					return EXEC_ELF_BROKEN;
				}
				rawSymbolCount = s.sh_size / s.sh_entsize;
			}
		}
		else if(SHT_STRTAB == s.sh_type) //string table header
		{
			if(NULL == stringTable)
			{
				stringTable = malloc(s.sh_size);
				fseek(f, (uintptr_t)(s.sh_offset), SEEK_SET);
				if(s.sh_size != fread(stringTable, 1, s.sh_size, f))
				{
					free(stringTable);
					if(NULL != symbolTable)
						free(symbolTable);
					return EXEC_ELF_BROKEN;
				}
			}

		}	
	}
	kernelSymbolTable = malloc(sizeof(struct KernelSymbol) * rawSymbolCount);
	for(uint32_t i = 0; i < rawSymbolCount; i++)
	{
		if(ELF32_ST_TYPE(symbolTable[i].st_info) == STT_FUNC
#ifdef ELF_KERNEL_PROVIDE_OBJECTS
		|| ELF32_ST_TYPE(symbolTable[i].st_info) == STT_OBJECT
#endif
		)
		{
			kernelSymbolTable[kernelSymbolCount].value = symbolTable[i].st_value;
			kernelSymbolTable[kernelSymbolCount].name = (char*)((uintptr_t)stringTable + symbolTable[i].st_name);
			printf("Got kernel symbol %s = 0x%X\n", kernelSymbolTable[kernelSymbolCount].name, kernelSymbolTable[kernelSymbolCount].value);
			kernelSymbolCount++;
		}
	}
	free(symbolTable);
	
}

static uint32_t elf_getKernelSymbol(const char *name)
{
	for(uint32_t i = 0; i < kernelSymbolCount; i++)
	{
		if(strcmp(name, kernelSymbolTable[i].name) == 0)
		{
			return kernelSymbolTable[i].value;;
		}
	}
	return 0;
}

/**
 * @brief Resolve symbol value
 * @param h ELF file header
 * @param table Index of symbol table
 * @param index Index inside a symbol table
 * @param symbolValue Returned symbol value
 * @return Error code
*/
static kError_t elf_getSymbolValue(struct Elf32_Ehdr *h, uint16_t table, uint32_t index, uint32_t *symbolValue)
{
	if((SHN_UNDEF == table) || (SHN_UNDEF == index))
	{
		*symbolValue = 0;
		return OK;
	}

	struct Elf32_Shdr *symTabHdr = elf_getSectionHeader(h, table); //get appropriate symbol table header
	uint32_t symTabEntries = symTabHdr->sh_size / symTabHdr->sh_entsize; //calculate number of entries
	if(index >= symTabEntries) //index exceeding table boundaries
	{
		return EXEC_ELF_BROKEN;
	}

	//get symbol structure from symbol table
	struct Elf32_Sym *symbol = &((struct Elf32_Sym*)((uintptr_t)h + symTabHdr->sh_offset))[index];

	if(SHN_UNDEF == symbol->st_shndx) //external symbol, external linking required
	{
		struct Elf32_Shdr *stringTabHdr = elf_getSectionHeader(h, symTabHdr->sh_link); //get string table header
		const char *name = (const char*)h + stringTabHdr->sh_offset + symbol->st_name; //get string for this symbol

		//perform linking with external symbol
		uint32_t value = elf_getKernelSymbol(name);

		if(0 == value) //symbol not found
		{
			if(ELF32_ST_BIND(symbol->st_info) & STB_WEAK) //this symbol was declared as weak and no definition was found
			{
				*symbolValue = value; //return weak value
				printf("Symbol %s has weak value of 0x%X.\n", name, *symbolValue);
				return OK; 
			}
			else //no definition found and this symbol is not weak: failure
			{
				printf("External symbol %s is not a kernel symbol.\nLoad failed.\n", name);
				return EXEC_ELF_UNDEFINED_EXTERNAL_SYMBOL;
			}
		}
		else //external symbol value found
		{
			*symbolValue = value;
			printf("External symbol %s at 0x%X\n", name, value);
			return OK;
		}
	}
	else if(SHN_ABS == symbol->st_shndx) //absolute symbol not affected by relocation
	{
		//st_value contains this absolute symbol value
		printf("Symbol has an absolute value of 0x%X.\n", symbol->st_value);
		*symbolValue = symbol->st_value;
		return OK;
	}
	else //symbol defined in the ELF image
	{
		//st_shndx is the section header index and is used to obatin sh_offset which is a section offset
		//st_value is the offset inside given section
		*symbolValue = (uintptr_t)h + elf_getSectionHeader(h, symbol->st_shndx)->sh_offset + symbol->st_value;
		printf("Symbol has a value of 0x%X.\n", *symbolValue);
		return OK;
	}

	return OK;
}

static kError_t elf_allocateBss(struct Elf32_Ehdr *h)
{
	struct Elf32_Shdr *sectionHdr;

	for(uint16_t i = 0; i < h->e_shnum; i++) //iterate through all sections
	{
		sectionHdr = elf_getSectionHeader(h, i); //get header

		if(SHT_NOBITS != sectionHdr->sh_type) //process only no-bits/bss sections
			continue;
		
		if(0 == sectionHdr->sh_size) //skip empty
			continue;

		if(sectionHdr->sh_flags & SHF_ALLOC) //check if memory for this section should be allocated in memory
		{
			//if so, allocate
			printf("Allocating %d bytes.\n", (int)sectionHdr->sh_size);
			//ask kernel for memory with size sh_size
			uintptr_t *m = malloc(sectionHdr->sh_size);
			//clear memory
			memset(m, 0, sectionHdr->sh_size);
			//store offset between allocated memory and ELF header in sh_offset
			sectionHdr->sh_offset = (uintptr_t)m - (uintptr_t)h;
		}
	}
	return OK;
}

static kError_t elf_relocateSymbol(struct Elf32_Ehdr *h, struct Elf32_Shdr *relSectionHdr, struct Elf32_Rel *relEntry)
{
	 //get target section header, i.e. the section where relocation will be applied
	struct Elf32_Shdr *targetSectionHdr = elf_getSectionHeader(h, relSectionHdr->sh_info);

	//get pointer to the point that needs to be relocated
	uint32_t *point = (uint32_t*)((uintptr_t)h + targetSectionHdr->sh_offset + relEntry->r_offset);

	uint32_t value = 0; //symbol value should default to 0

	//check if symbol to be relocated is defined
	if(SHN_UNDEF != ELF32_R_SYM(relEntry->r_info))
	{
		//get it's value
		kError_t ret = elf_getSymbolValue(h, relSectionHdr->sh_link, ELF32_R_SYM(relEntry->r_info), &value);
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
			printf("Relocation failed: relocation type is unsupported.\n");
			return EXEC_ELF_UNSUPPORTED_TYPE;
			break;
	}
	return OK;
}


static kError_t elf_performRelocation(struct Elf32_Ehdr *h)
{
	kError_t ret = OK;
	ret = elf_allocateBss(h); //allocate all no-bits sections first
	if(OK != ret) 
		return ret;
	
	struct Elf32_Shdr *sectionHdr;

	for(uint16_t i = 0; i < h->e_shnum; i++) //iterate thorugh all sections
	{
		sectionHdr = elf_getSectionHeader(h, i); //get header

		if(SHT_REL != sectionHdr->sh_type) //skip sections other than relocation
			continue;

		struct Elf32_Rel *entries = ((struct Elf32_Rel*)((uintptr_t)h + sectionHdr->sh_offset)); //get entry table
		for(uint32_t k = 0; k < (sectionHdr->sh_size / sectionHdr->sh_entsize); k++) //loop for every entry
		{
			ret = elf_relocateSymbol(h, sectionHdr, &entries[k]); //relocate symbol
			if(OK != ret)
				return ret;
		}
	}
	return OK;
}


kError_t Elf_load(char *name, uint32_t *entryPoint)
{
	kError_t ret = OK;
	FILE *f = fopen(name, "rb");
	fseek(f, 0L, SEEK_END);
	uint32_t size = ftell(f);
	void *d = malloc(size);
	fseek(f, 0L, SEEK_SET);

	size_t m = fread(d, 1, size, f);

	struct Elf32_Ehdr *h = (struct Elf32_Ehdr*)d; //get ELF header
	if(h->ei_mag[0] != ELFMAG0 || h->ei_mag[1] != ELFMAG1 || h->ei_mag[2] != ELFMAG2 || h->ei_mag[3] != ELFMAG3) //check for magic number
		return EXEC_ELF_BAD_FORMAT;

	if(h->ei_class != ELFCLASS32) //check for architecture
		return EXEC_ELF_BAD_ARCHITECTURE;

	if(h->ei_data != ELFDATA2LSB) //accept only little endian
		return EXEC_ELF_BAD_ENDIANESS;

	if(h->e_machine != EM_386)
		return EXEC_ELF_BAD_INSTRUCTION_SET;

	if(h->ei_version != EV_CURRENT)
		return EXEC_ELF_BROKEN;

	if(h->e_version != EV_CURRENT)
		return EXEC_ELF_BROKEN;

	switch(h->e_type)
	{
		case ET_EXEC:
			//ret = elf_loadProgramSegments(h, name);
			if(ret != OK)
				return ret;
			break;

		case ET_REL:
			ret = elf_performRelocation(h);
			if(ret != OK)
				return ret;
			break;

		default:
			return EXEC_ELF_UNSUPPORTED_TYPE;
			break;
	}

	free(d);

	return 0;
}
