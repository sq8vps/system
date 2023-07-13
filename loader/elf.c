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


/**
 * @brief Temporary buffer for ELF examination and loading executables
 * 
 * This buffer is used only as a temporary storage for ELF header
 * and program header during loading a executable (probably kernel exclusively).
 * When loading other files it is used only for ELF header examination.
*/
uint8_t buf[ELF32_EHDR_SIZE > ELF32_PHDR_SIZE ? ELF32_EHDR_SIZE : ELF32_PHDR_SIZE];


error_t Elf_loadExec(const uint8_t *name, uintptr_t *entryPoint)
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