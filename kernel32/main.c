#include <stdint.h>
#include "it.h"
#include "../drivers/vga/vga.h"

void main(void)
{
	uint32_t *pageUsageTable;
	asm volatile("mov %0, eax" : "=d" (pageUsageTable) : : );
	
	disp_clear();
	printf("Jadro!!!\n");
	It_init();
	
	//asm volatile("int 0x30");

	while(1);;
}
