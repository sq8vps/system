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
#include "mm/gdt.h"
#include "mm/dynmap.h"
#include "hal/hal.h"
#include "ke/task.h"
#include "ke/tss.h"
#include "ke/sched.h"
#include "ke/mutex.h"

#include "../drivers/vga/vga.h"

extern uintptr_t _KERNEL_TEMPORARY_STACK_ADDRESS; //linker-defined temporary kernel stack address symbol

struct KernelEntryArgs kernelArgs; //copy of kernel entry arguments

//TODO:
//dealokacja sterty

static KeSpinLock_t bootMutex;

NORETURN void main(struct KernelEntryArgs args)
{	
	//store kernel arguments locally as the previous stack will be destroyed
	Cm_memcpy(&kernelArgs, &args, sizeof(args));
	
	uintptr_t kernelStackAddress = (uintptr_t)&_KERNEL_TEMPORARY_STACK_ADDRESS; //get temporary stack address
	asm volatile("mov esp, %0" : : "m" (kernelStackAddress) : ); //set up stack top address in ESP register
	asm volatile("mov ebp, %0" : : "m" (kernelStackAddress) : ); //set up base address in EBP register

	MmGdtInit();
	MmGdtApplyFlat();
	MmInitPhysicalAllocator(&kernelArgs);
	MmInitVirtualAllocator();

	MmInitDynamicMemory(&kernelArgs);
	disp_init();
	disp_clear();

	HalInitInterruptController();
	It_init(); //initialize interrupts and exceptions

	KePrepareTSS(0);

	KeSchedulerStart();
	
	//printf("OK\n");
	while(1);;
}
