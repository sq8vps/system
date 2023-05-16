#include "elf.h"

#include "fat.h"
#include "mm.h"
#include "disp.h"
#include "../cdefines.h"

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
#define ELF32_EHDR_SIZE (sizeof(struct Elf32_Ehdr))

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
#define ELF32_PHDR_SIZE (sizeof(struct Elf32_Phdr))

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

/**
 * The bootloader loads some driver and other files that kernel requires,
 * e.g. disk and filesystem drivers, as the kernel itself does not include any
 * All loaded files metadata is stored in this table and passed to the kernel
*/
#define KERNEL_RAW_ELF_TABLE_MAX 10 //max number of entries in the table. Probably not too many needed
struct KernelRawELFEntry rawELFTable[KERNEL_RAW_ELF_TABLE_MAX]; //the table itself
uintptr_t rawELFTableSize = 0; //number of elements in table

/**
 * @brief Temporary buffer for ELF examination and loading executables
 * 
 * This buffer is used only as a temporary storage for ELF header
 * and program header during loading a executable (probably kernel exclusively).
 * When loading other files it is used only for ELF header examination.
*/
uint8_t buf[ELF32_EHDR_SIZE > ELF32_PHDR_SIZE ? ELF32_EHDR_SIZE : ELF32_PHDR_SIZE];

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


error_t Elf_loadExec(const uint8_t *name, uint32_t *entryPoint)
{
	error_t ret = OK;
	ret = Fat_readFile(&fatDisk, (char*)name, 0, ELF32_EHDR_SIZE, buf); //read ELF header
	if(ret != OK)
		return ret;

	struct Elf32_Ehdr *h = (struct Elf32_Ehdr*)buf; //get buffer pointer

	if(ELFMAG0 != h->ei_mag[0] || ELFMAG1 != h->ei_mag[1] || ELFMAG2 != h->ei_mag[2] || ELFMAG3 != h->ei_mag[3]) //check for magic number
		return ELF_BAD_FORMAT;

	if(ELFCLASS32 != h->ei_class) //check for architecture
		return ELF_BAD_ARCHITECTURE;

	if(ELFDATA2LSB != h->ei_data) //accept only little endian
		return ELF_BAD_ENDIANESS;

	if(EM_386 != h->e_machine)
		return ELF_BAD_INSTRUCTION_SET;

	if(EV_CURRENT != h->ei_version)
		return ELF_BROKEN;

	if(EV_CURRENT != h->e_version)
		return ELF_BROKEN;

	if(ET_EXEC != h->e_type)
		return ELF_UNSUPPORTED_TYPE;

	*entryPoint = (h->e_entry); //store entry point

	uint16_t phentsize = h->e_phentsize; //store program header parameters, as the main ELF header will be overwritten
	uint16_t phnum = h->e_phnum;
	uint32_t phoff = h->e_phoff;

	struct Elf32_Phdr *p = (struct Elf32_Phdr *)(buf); //get program header entry pointer
	for(uint16_t i = 0; i < phnum; i++)
	{
		ret = Fat_readFile(&fatDisk, (uint8_t*)name, phoff + i * phentsize, phentsize, buf); //read program header entry
		if(OK != ret)
			return ret;

		if(PT_LOAD != p->p_type) //only PT_LOAD type
			continue;
		if(0 == p->p_memsz) //skip empty segments
			continue;
		if(p->p_filesz > p->p_memsz) //file size must not be bigger than memory size
			return ELF_BROKEN;

		ret = Mm_allocateEx(p->p_vaddr, p->p_memsz / MM_PAGE_SIZE + ((p->p_memsz % MM_PAGE_SIZE) ? 1 : 0), 0); //allocate page(s)
		if(ret != OK)
			return ret;

		ret = Fat_readFile(&fatDisk, (uint8_t*)name, p->p_offset, p->p_filesz, (uint8_t*)(p->p_vaddr)); //read segment
		if(ret != OK)
			return ret;

		for(uint32_t i = p->p_filesz; i < p->p_memsz; i++) //fill rest with zeros
			*((uint8_t*)(p->p_vaddr + i)) = 0;
	}

	return OK;
}

/**
 * @brief Allocate no-bits/BSS sections
 * @param h ELF header pointer
 * @param vaddr Virtual address to allocate memory at - must be page aligned
 * @param allocated Size of memory actually allocated
 * @return Error code
 * 
 * This function is required when loading raw ELF files.
 * ELF files may contain sections that must be allocated in memory,
 * but are not contained within the ELF file. These are mostly BSS sections.
*/
static error_t elf_allocateBss(struct Elf32_Ehdr *h, uintptr_t vaddr, uintptr_t *allocated)
{
	*allocated = 0;
	error_t ret = OK;
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
			//allocate memory
			uintptr_t requiredPages = sectionHdr->sh_size / MM_PAGE_SIZE + (sectionHdr->sh_size % MM_PAGE_SIZE) ? 1 : 0;
			if(OK != (ret = Mm_allocateEx(vaddr, requiredPages, 0)))
				return ret;
			//clear memory
			for(uintptr_t i = 0; i < sectionHdr->sh_size; i++)
				*(uint8_t*)(vaddr) = 0;
			//store offset between allocated memory and ELF header in sh_offset
			sectionHdr->sh_offset = vaddr - (uintptr_t)h;
			
			*allocated += (requiredPages * MM_PAGE_SIZE);
			vaddr += (requiredPages * MM_PAGE_SIZE); //the next virtual address must be page-aligned
		}
	}
	return OK;
}

error_t Elf_loadRaw(const char *name, uint32_t *vaddr)
{
	if(strlen(name) > KERNEL_RAW_ELF_ENTRY_NAME_SIZE)
		return ELF_NAME_TOO_LONG;
	
	if(KERNEL_RAW_ELF_TABLE_MAX == rawELFTableSize)
		return ELF_RAW_TABLE_FULL;
	
	error_t ret = OK;
	ret = Fat_readFile(&fatDisk, (uint8_t*)name, 0, ELF32_EHDR_SIZE, buf); //read ELF header first
	if(ret != OK)
		return ret;

	struct Elf32_Ehdr *h = (struct Elf32_Ehdr*)buf; //get buffer pointer

	if(ELFMAG0 != h->ei_mag[0] || ELFMAG1 != h->ei_mag[1] || ELFMAG2 != h->ei_mag[2] || ELFMAG3 != h->ei_mag[3]) //check for magic number
		return ELF_BAD_FORMAT;

	if(ELFCLASS32 != h->ei_class) //check for architecture
		return ELF_BAD_ARCHITECTURE;

	if(ELFDATA2LSB != h->ei_data) //accept only little endian
		return ELF_BAD_ENDIANESS;

	if(EM_386 != h->e_machine)
		return ELF_BAD_INSTRUCTION_SET;

	if(EV_CURRENT != h->ei_version)
		return ELF_BROKEN;

	if(EV_CURRENT != h->e_version)
		return ELF_BROKEN;

	uint32_t size = 0;
	if(OK != (ret = Fat_getFileSize(&fatDisk, (uint8_t*)name, &size))) //get file size first
		return ret;

	uint32_t requiredPages = size / MM_PAGE_SIZE + ((size % MM_PAGE_SIZE) ? 1 : 0);
	if(OK != (ret = Mm_allocateEx(*vaddr, requiredPages, 0))) //allocate page(s)
		return ret;

	if(OK != (ret = Fat_readWholeFile(&fatDisk, (uint8_t*)name, (uint8_t*)(*vaddr), &size))) //read file
		return ret;
	
	h = (struct Elf32_Ehdr*)(*vaddr); //get ELF header again

	uint32_t allocated = 0;
	if(OK != (ret = elf_allocateBss(h, *vaddr + requiredPages * MM_PAGE_SIZE, &allocated))) //allocate all no-bits/bss sections
		return ret;

	//add file to table
	rawELFTable[rawELFTableSize].size = (requiredPages * MM_PAGE_SIZE) + allocated;
	rawELFTable[rawELFTableSize].vaddr = *vaddr;
	memcpy(rawELFTable[rawELFTableSize].name, name, strlen(name));
	*vaddr += rawELFTable[rawELFTableSize].size; //update virtual address for the next file

	rawELFTableSize++;

	return OK;
}

struct KernelRawELFEntry* Elf_getRawELFTable(uintptr_t *tableSize)
{
	*tableSize = rawELFTableSize;
	return rawELFTable;
}

//address to load the next driver ELF file to
uintptr_t nextDriverVaddr = MM_DRIVERS_START_ADDRESS;

//address to load the next other ELF file to
uintptr_t nextOtherVaddr = MM_OTHER_START_ADDRESS;

error_t Elf_loadDriver(const char *name)
{
	return Elf_loadRaw(name, &nextDriverVaddr);
}

error_t Elf_loadOther(const char *name)
{
	return Elf_loadRaw(name, &nextOtherVaddr);
}