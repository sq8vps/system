#include <stdint.h>

void main(void)
{
	uint32_t *pageUsageTable;
	asm volatile("mov %0, eax" : "=d" (pageUsageTable) : : );
	while(1);;
}
