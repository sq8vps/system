#ifndef LOADER_ELF_H
#define LOADER_ELF_H

/**
 * @file elf.h
 * @brief ELF file definitions and manipulation routines
 * 
 * Provides ELF file definitions and manipulation routines
 * 
 * 
*/

#include <stdint.h>
#include "defines.h"

/**
 * @defgroup elf ELF definitions and mainpulation routines
 * @ingroup exec
 * @{
*/

//ELF type
enum Elf32_e_type
{
	ET_NONE = 0,
	ET_REL = 1,
	ET_EXEC = 2,
	ET_DYN = 3,
	ET_CORE = 4
};

//ELF machine
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

//ELF file version
enum Elf32_e_version
{
	EV_NONE = 0,
	EV_CURRENT = 1,
};

//ELF magic numbers
enum Elf32_ei_mag
{
	ELFMAG0 = 0x7f,
	ELFMAG1 = 'E',
	ELFMAG2 = 'L',
	ELFMAG3 = 'F',
};

//ELF class (32/64 bits)
enum Elf32_ei_class
{
	ELFCLASSNONE = 0,
	ELFCLASS32 = 1,
	ELFCLASS64 = 2,
};

//ELF endianess
enum Elf32_ei_data
{
	ELFDATANONE = 0,
	ELFDATA2LSB = 1,
	ELFDATA2MSB = 2,
};

//ELF file header
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

//ELF program header type
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

//ELF program header
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

//ELF section header type
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

//ELF section header flags
enum Elf32_sh_flags
{
	SHF_WRITE = 0x1,
	SHF_ALLOC = 0x2,
	SHF_EXECINSTR = 0x4,
	SHF_MASKPROC = 0xf0000000,
};

//ELF section header defines
#define SHN_UNDEF (0)
#define SHN_ABS (0xfff1)

//ELF section header
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

//ELF symbol entry
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

//ELF relocation entry
struct Elf32_Rel
{
	uint32_t r_offset;
	uint32_t r_info;
} __attribute__ ((packed));

//ELF relocation with addend entry
struct Elf32_Rela
{
	uint32_t r_offset;
	uint32_t r_info;
	int32_t r_addend;
} __attribute__ ((packed));

//ELF relocation helper macros
#define ELF32_R_SYM(i)	((i) >> 8)
#define ELF32_R_TYPE(i)	((uint8_t)(i))
#define ELF32_R_INFO(s, t) (((s) << 8) + (uint8_t)(t))

//ELF relocation types
enum Elf32_Rel_types {
	R_386_NONE = 0, //no relocation
	R_386_32 = 1, //symbol + addend
	R_386_PC32 = 2,  //symbol + addend - section offset
};

/**
 * @brief ELF external symbol resolver function typedef
*/
typedef uintptr_t (*ExElfResolver_t)(const char *name);

/**
 * @brief Get ELF32 section header
 * @param h ELF32 file header
 * @param n Section index
 * @return Section header address
*/
struct Elf32_Shdr* ExGetElf32SectionHeader(struct Elf32_Ehdr *h, uint16_t n);

/**
 * @brief Verify ELF32 main header
 * @param h ELF32 file header
 * @return Error code (OK on successful verification)
*/
STATUS ExVerifyElf32Header(struct Elf32_Ehdr *h);

/**
 * @brief Get ELF32 symbol value by name
 * @param h ELF file header
 * @param name Symbol name
 * @param symbolValue Returned symbol value
 * @return Error code
 * @attention All symbols must be relocated and linked first
*/
STATUS ExGetElf32SymbolValueByName(struct Elf32_Ehdr *h, char *name, uintptr_t *symbolValue);

/**
 * @brief Resolve ELF32 symbol value for given symbol table and index
 * @param h ELF file header
 * @param table Index of symbol table
 * @param index Index inside a symbol table
 * @param symbolValue Returned symbol value
 * @param resolver External symbol resolver function
 * @return Error code
*/
STATUS ExGetElf32SymbolValue(struct Elf32_Ehdr *h, uint16_t table, uint32_t index, uintptr_t *symbolValue, ExElfResolver_t resolver);

/**
 * @brief Relocate ELF32 symbol
 * @param h ELF file header
 * @param relSectionHdr Relocation section header
 * @param relEntry Relocation entry
 * @param resolver External symbol resolver function
*/
STATUS ExRelocateElf32Symbol(struct Elf32_Ehdr *h, struct Elf32_Shdr *relSectionHdr, struct Elf32_Rel *relEntry, ExElfResolver_t resolver);

/**
 * @brief Perform full ELF32 symbol relocation
 * @param h ELF file header
 * @param resolver External symbol resolver function
*/
STATUS ExPerformElf32Relocation(struct Elf32_Ehdr *h, ExElfResolver_t resolver);

/**
 * @}
*/

#endif
