#include <stdint.h>
#include "it/it.h"
#include "mm/mm.h"
#include "mm/heap.h"
#include "../cdefines.h"
#include "exec/exec.h"
#include "common.h"
#include "exec/kdrv.h"
#include "io/display.h"

#include "../drivers/vga/vga.h"

extern uint32_t _KERNEL_STACK_ADDRESS; //linker-defined kernel stack address symbol

 __attribute__ ((noreturn))
void main(struct KernelEntryArgs args)
{
	uint32_t kernelStackAddress = (uint32_t)&_KERNEL_STACK_ADDRESS; //get stack address
	asm volatile("mov esp, %0" : : "m" (kernelStackAddress) : ); //set up stack top address in ESP register
	
	It_init(); //initialize interrupts and exceptions
	Mm_init(args.pageUsageTable, args.kernelPageDir); //initialize memory management module
	
	disp_clear();
	
	// for(uint32_t i = 0; i < args.rawElfTableSize; i++)
	// {
	// 	if(0 == Cm_strcmp(args.rawElfTable[i].name, KERNEL_FILE_NAME))
	// 		printf("Returned %d\n", (int)Ex_loadKernelSymbols(args.rawElfTable[i].vaddr));
	// 	if(0 == Cm_strcmp(args.rawElfTable[i].name, "tmvga.drv"))
	// 		printf("Returned %d\n", (int)Ex_loadPreloadedDriver(args.rawElfTable[i].vaddr, args.rawElfTable[i].size));
	// }

	printf("OK\n");

	while(1);;
}
