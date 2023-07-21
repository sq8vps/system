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
#include "bootvga/bootvga.h"
#include "ke/panic.h"

#include "hal/cpu.h"
#include "hal/interrupt.h"

extern uintptr_t _KERNEL_TEMPORARY_STACK_ADDRESS; //linker-defined temporary kernel stack address symbol

static struct KernelEntryArgs kernelArgs; //copy of kernel entry arguments

//TODO:
//dealokacja sterty

NORETURN static void KeInit(void)
{	
	//initialize core kernel modules
	//these function do not return any values, but will panic on any failure
	MmGdtInit();
	MmGdtApplyFlat();
	MmInitPhysicalAllocator(&kernelArgs);
	MmInitVirtualAllocator();
	MmInitDynamicMemory(&kernelArgs);
	//boot VGA driver can be initialized when dynamic memory allocator is available
	if(OK != BootVgaInit())
		ItHardReset();
	
	if(OK != HalInit())
		KePanicEx(BOOT_FAILURE, HAL_INIT_FAILURE, 0, 0, 0);

	KePrepareTSS(0);

	ItInit(); //initialize interrupts and exceptions

	HalListCpu();

	//KeSchedulerStart();
	
	//printf("OK\n");
	while(1);;
}

NORETURN void KeEntry(struct KernelEntryArgs args)
{
	//store kernel arguments locally as the previous stack will be destroyed
	CmMemcpy(&kernelArgs, &args, sizeof(args));
	
	uintptr_t kernelStackAddress = (uintptr_t)&_KERNEL_TEMPORARY_STACK_ADDRESS; //get temporary stack address
	ASM("mov esp, %0" : : "m" (kernelStackAddress) : ); //set up stack top address in ESP register
	ASM("mov ebp, %0" : : "m" (kernelStackAddress) : ); //set up base address in EBP register
	//do not declare any local variables here
	//stack and base pointers were changed and the compiler is not aware of this fact
	//just jump to the initialization routine
	KeInit();
}