#include "elf.h"
#include "defines.h"
#include "fat.h"

struct Elf32_header
{
        uint8_t magic[4];
        uint8_t cpuType;
        uint8_t endianess;
        uint8_t headerVer;
        uint8_t abi;
        uint64_t unused;
        uint16_t elfType;
        uint16_t instrSet;
        uint32_t elfVer;
        uint32_t entry;
        uint32_t programHeaderPos;
        uint32_t sectionHeaderPos;
        uint32_t flags;
        uint16_t headerSize;
        uint16_t programHeaderEntrySize;
        uint16_t programHeaderEntryCount;
        uint16_t sectionHeaderEntrySize;
        uint16_t sectionHeaderEntryCount;
        uint16_t sectionHeaderNamesIndex;
} __attribute__ ((packed()));

uint8_t buf[4096] = {0};



error_t Elf_load(uint8_t *name, uint8_t kernelSpace, uint32_t *entryPoint)
{
	error_t ret = OK;
	ret = Fat_readFile(&fatDisk, name, 0, 64, buf);
	if(ret != OK)
		return ret;

	struct Elf32_header *h = (struct Elf32_header*)buf;
	if(h->magic[0] != 0x7F || h->magic[1] != 'E' || h->magic[2] != 'L' || h->magic[3] != 'F') //check for magic number
		return ELF_NOT_ELF;

	if(h->cpuType != 1) //check for architecture
		return ELF_BAD_ARCHITECTURE;

	if(h->endianess != 1) //accept only little endian
		return ELF_INCOMPATIBLE;

	if(h->elfType != 0x02)
		return ELF_INCOMPATIBLE;

	if(h->instrSet != 0x03)
		return ELF_INCOMPATIBLE;

	entryPoint = (uint32_t*)h->entry;

}
