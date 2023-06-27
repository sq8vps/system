#include <stdint.h>
#include "it/it.h"
#include "mm/palloc.h"
#include "mm/valloc.h"
#include "mm/heap.h"
#include "mm/mm.h"
#include "../cdefines.h"
#include "exec/exec.h"
#include "common.h"
#include "exec/kdrv.h"
#include "io/display.h"
#include "mm/gdt.h"
#include "mm/dynmap.h"

#include "../drivers/vga/vga.h"

extern uintptr_t _KERNEL_STACK_ADDRESS; //linker-defined kernel stack address symbol

struct KernelEntryArgs kernelArgs; //copy of kernel entry arguments

NORETURN void main(struct KernelEntryArgs args)
{	
	//store kernel arguments locally as the previous stack will be destroyed
	Cm_memcpy(&kernelArgs, &args, sizeof(args));

	disp_clear();

	GdtInit();
	GdtApplyFlat();
	It_init(); //initialize interrupts and exceptions
	MmInitPhysicalAllocator(&kernelArgs);
	uintptr_t kernelStackAddress = (uintptr_t)&_KERNEL_STACK_ADDRESS; //get stack address
	asm volatile("mov esp, %0" : : "m" (kernelStackAddress) : ); //set up stack top address in ESP register
	asm volatile("mov ebp, %0" : : "m" (kernelStackAddress) : ); //set up stack base address in EBP register
	MmInitVirtualAllocator();
	MmMapArbitraryKernelMemory(0xB8000, 0xB8000, MM_PAGE_FLAG_WRITABLE);
	MmMapArbitraryKernelMemory(0xB9000, 0xB9000, MM_PAGE_FLAG_WRITABLE);
	MmInitDynamicMemory(&kernelArgs);
	
	printf("OK\n");
	while(1);;
}
