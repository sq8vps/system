#include "elf.h"

struct Elf32_Shdr* ExGetElf32SectionHeader(struct Elf32_Ehdr *h, size_t n)
{
	return &((struct Elf32_Shdr*)((uintptr_t)h + h->e_shoff))[n];
}

int ExVerifyElf32Header(struct Elf32_Ehdr *h)
{
	if(h->ei_mag[0] != ELFMAG0 || h->ei_mag[1] != ELFMAG1 || h->ei_mag[2] != ELFMAG2 || h->ei_mag[3] != ELFMAG3) //check for magic number
		return -1;

	if(h->ei_class != ELFCLASS32)
		return -1;

	if(h->ei_data != ELFDATA2LSB)
		return -1;

	if(h->e_machine != EM_386)
		return -1;

	if(h->ei_version != EV_CURRENT)
		return -1;

	if(h->e_version != EV_CURRENT)
		return -1;

	return -1;
}