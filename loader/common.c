#include "common.h"

void *memcpy(void *to, const void *from, uint64_t num)
{
	for(uint64_t i = 0; i < num; i++)
	{
		*((uint8_t*)(to + i)) = *((uint8_t*)(from + i));
	}
	return to;
}


/**
 * \brief Aligns selected address to the nearest possible page start address (always rounding up)
 * \param[in] addr Address to be aligned
 * \return Address aligned to PAGE_SIZE
 */
uint64_t alignToPage(uint64_t addr)
{
	if(addr % PAGE_SIZE == 0) return addr; //the address is already aligned, so return it

	uint64_t r = addr / PAGE_SIZE; //check for the full pages before the address
	r++; //add 1 to round up to the nearest page
	return r * PAGE_SIZE; //page rounded up * page size
}


uint64_t pow10(uint8_t x)
{
	uint64_t ret = 1;
	for(; x > 0; x--)
	{
		ret *= 10;
	}
	return ret;
}

uint32_t bytesToUint32LE(uint8_t *in)
{
	uint32_t ret;
	ret = (uint32_t)in[0];
	ret |= ((uint32_t)in[1] << 8);
	ret |= ((uint32_t)in[2] << 16);
	ret |= ((uint32_t)in[3] << 24);
	return ret;
}

uint16_t bytesToUint16LE(uint8_t *in)
{
	uint32_t ret;
	ret = (uint16_t)in[0];
	ret |= ((uint16_t)in[1] << 8);
	return ret;
}

uint32_t strlen(const uint8_t *s)
{
	uint32_t ret = 0;
	while(*s != '\0')
	{
		ret++;
		s++;
		if(ret == 0xFFFFFFFF)
			break;
	}
	return ret;
}

uint8_t strcmp(const uint8_t *s1, const uint8_t *s2)
{
	if(strlen(s1) != strlen(s2)) //different lengths, string are not the same
		return 1;

	while(*s1 != 0)
	{
		if(*s1 != *s2)
			return 1;
		s1++;
		s2++;
	}
	return 0;
}

uint8_t strncmp(const uint8_t *s1, const uint8_t *s2, uint32_t len)
{

	while(*s1 != 0)
	{
		if(*s1 != *s2)
			return 1;
		s1++;
		s2++;
		if(len == 0)
			return 0;
		len--;
	}
	return 0;
}
