#ifndef ELF_H
#define ELF_H

#include <stdint.h>
#include <stddef.h>

#ifdef __GNUC__
#define PACKED __attribute__((packed))
#else
#error Must define macros for some compiler-specific attributes
#endif


/**
 * @brief ELF image type
 */
enum Elf32_e_type
{
	ET_NONE = 0,
	ET_REL = 1,
	ET_EXEC = 2,
	ET_DYN = 3,
	ET_CORE = 4
};

/**
 * @brief ELF target machine
 */
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

/**
 * @brief ELF file version
 */
enum Elf32_e_version
{
	EV_NONE = 0,
	EV_CURRENT = 1,
};

/**
 * @brief ELF magic numbers
 */
enum Elf32_ei_mag
{
	ELFMAG0 = 0x7f,
	ELFMAG1 = 'E',
	ELFMAG2 = 'L',
	ELFMAG3 = 'F',
};

/**
 * @brief ELF class (32/64 bits)
 */
enum Elf32_ei_class
{
	ELFCLASSNONE = 0,
	ELFCLASS32 = 1,
	ELFCLASS64 = 2,
};

/**
 * @brief ELF endianess
 */
enum Elf32_ei_data
{
	ELFDATANONE = 0,
	ELFDATA2LSB = 1,
	ELFDATA2MSB = 2,
};

/**
 * @brief ELF file header
 */
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
} PACKED;

/**
 * @brief ELF program header type
 */
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

#define PF_X 0x1
#define PF_W 0x2
#define PF_R 0x4

/**
 * @brief ELF program header
 */
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
} PACKED;

/**
 * @brief ELF symbol entry
 */
struct Elf32_Sym
{
	uint32_t st_name;
	uint32_t st_value;
	uint32_t st_size;
	uint8_t st_info;
	uint8_t st_other;
	uint16_t st_shndx;
} PACKED;

//st_info field manipulation macros
#define ELF32_ST_BIND(i) ((i) >> 4)
#define ELF32_ST_TYPE(i) ((i) & 0xf)
#define ELF32_ST_INFO(b, t) (((b) << 4) + ((t) & 0xf))
#define ELF32_ST_VISIBILITY(i) ((i) & 0x3)

//ELF symbol flags
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

#define STV_DEFAULT 0
#define STV_INTERNAL 1
#define STV_HIDDEN 2
#define STV_PROTECTED 3

/**
 * @brief ELF relocation entry
 */
struct Elf32_Rel
{
	uint32_t r_offset;
	uint32_t r_info;
} PACKED;

/**
 * @brief ELF relocation entry with addend
 */
struct Elf32_Rela
{
	uint32_t r_offset;
	uint32_t r_info;
	int32_t r_addend;
} PACKED;

//ELF relocation helper macros
#define ELF32_R_SYM(i)	((i) >> 8)
#define ELF32_R_TYPE(i)	((uint8_t)(i))
#define ELF32_R_INFO(s, t) (((s) << 8) + (uint8_t)(t))

/**
 * @brief ELF relocation type
 */
enum Elf32_Rel_types 
{
	R_386_NONE = 0, //no relocation
	R_386_32 = 1, //symbol + addend
	R_386_PC32 = 2,  //symbol + addend - section offset
	R_386_GLOB_DAT = 6, //set target symbol in GOT
	R_386_JMP_SLOT = 7, //set target symbol in PLT
};

/**
 * @brief ELF dynamic section entry
 */
struct Elf32_Dyn
{
	int32_t d_tag;
	union
	{
		uint32_t d_val;
		uintptr_t d_ptr;
	} d_un;
} PACKED;

enum Elf32_Dyn_type
{
	DT_NULL = 0, //null entry
	DT_NEEDED = 1, //needed library name
	DT_PLTRELSZ = 2, //size of PLT-associated relocation entries
	DT_PLTGOT = 3, //PLT address
	DT_HASH = 4, //symbol hash table address
	DT_STRTAB = 5, //string table
	DT_SYMTAB = 6, //symbol table
	DT_RELA = 7, //address of the relocation table (relocation with addend)
	DT_RELASZ = 8, //size of dynamic relocation entries (relocation with addend)
	DT_STRSZ = 10, //string table size
	DT_SYMENT = 11, //symbol table entry count
	DT_REL = 17, //address of the relocation table (relocation without addend)
	DT_RELSZ = 18, //size of dynamic relocation entries (relocation without addend)
	DT_PLTREL = 20, //PLT relocation entry type
	DT_JMPREL = 23, //PLT relocation entry table address
};

/**
 * @brief Verify ELF32 main header
 * @param h ELF32 file header
 * @return 0 on success, -1 otherwise
*/
int DlVerifyElf32Header(struct Elf32_Ehdr *h);

/**
 * @}
*/

#endif
