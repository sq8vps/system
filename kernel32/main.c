#include <stdint.h>
#include "it/it.h"
#include "mm/mm.h"
#include "mm/heap.h"
#include "../cdefines.h"
//#include "../drivers/vga/vga.h"

extern uint32_t _KERNEL_STACK_ADDRESS; //linker-defined kernel stack address symbol

 __attribute__ ((noreturn))
void main(struct KernelEntryArgs args)
{
	uint32_t kernelStackAddress = (uint32_t)&_KERNEL_STACK_ADDRESS; //get stack address
	asm volatile("mov esp, %0" : : "m" (kernelStackAddress) : ); //set up stack top address in ESP register
	
	It_init(); //initialize interrupts and exceptions
	Mm_init(args.pageUsageTable, args.kernelPageDir); //initialize memory management module
	
	//disp_clear();


	while(1);;
}
