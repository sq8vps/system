#include "elf.h"

#include "fat.h"
#include "mm.h"
#include "disp.h"

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

struct Elf32_prog_head_entry
{
	uint32_t type;
	uint32_t offset;
	uint32_t vAddr;
	uint32_t pAddr;
	uint32_t fileSize;
	uint32_t memSize;
	uint32_t flags;
	uint32_t align;
} __attribute__ ((packed()));

uint8_t buf[4096] = {0};


error_t Elf_load(uint8_t *name, uint32_t *entryPoint)
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

	*entryPoint = (h->entry);

	uint16_t progHeadEntSize = h->programHeaderEntrySize;
	uint16_t progHeadEntCount = h->programHeaderEntryCount;

	ret = Fat_readFile(&fatDisk, name, h->programHeaderPos, progHeadEntSize * progHeadEntCount, buf); //read program header entries
	if(ret != OK)
		return ret;

	struct Elf32_prog_head_entry *p;
	for(uint16_t i = 0; i < progHeadEntCount; i++)
	{
		p = (struct Elf32_prog_head_entry *)(buf + i * progHeadEntSize); //get next program header entry

		if(p->type != 0x01) //only PT_LOAD type
			continue;
		if(p->memSize == 0) //skip empty segments
			continue;
		if(p->fileSize > p->memSize) //file size must not be bigger than memory size
			return ELF_BROKEN;

		ret = Mm_allocateEx(p->vAddr, p->memSize / MM_PAGE_SIZE + ((p->memSize % MM_PAGE_SIZE) ? 1 : 0), MM_PAGE_FLAG_SUPERVISOR); //allocate page(s)
		if(ret != OK)
			return ret;

		ret = Fat_readFile(&fatDisk, name, p->offset, p->fileSize, (uint8_t*)(p->vAddr)); //read segment
		if(ret != OK)
			return ret;

		for(uint32_t i = p->fileSize; i < p->memSize; i++) //fill rest with zeros
			*((uint8_t*)(p->vAddr + i)) = 0;
	}

	return OK;
}
