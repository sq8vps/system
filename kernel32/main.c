#include <stdint.h>
#include "it/it.h"
#include "mm/palloc.h"
#include "mm/valloc.h"
#include "mm/heap.h"
#include "mm/mm.h"
#include "../cdefines.h"
#include "ex/exec.h"
#include "common.h"
#include "ex/kdrv.h"
#include "mm/gdt.h"
#include "mm/dynmap.h"
#include "hal/hal.h"
#include "ke/task/task.h"
#include "ke/task/tss.h"
#include "ke/sched/sched.h"
#include "ke/core/mutex.h"
#include "sdrv/bootvga/bootvga.h"
#include "ke/core/panic.h"
#include "io/fs/vfs.h"
#include "io/fs/fs.h"
#include "hal/cpu.h"
#include "hal/interrupt.h"
#include "sdrv/initrd/initrd.h"
#include "ex/ksym.h"
#include "defines.h"
#include "ke/task/task.h"
#include "ke/sched/sched.h"
#include "ke/sched/sleep.h"
#include "sdrv/fpu.h"
#include "sdrv/sse.h"

extern uintptr_t _KERNEL_INITIAL_STACK_ADDRESS; //linker-defined temporary kernel stack address symbol

static struct KernelEntryArgs kernelArgs; //copy of kernel entry arguments

//TODO:
//dealokacja sterty

KeMutex s = KeMutexInitializer;
KeSemaphore sem = KeSemaphoreInitializer;

void task1(void *c)
{
	while(1)
	{
		KeAcquireSemaphore(&sem);
		PRINT("1");
		KePutTaskToSleep(KeGetCurrentTask(), MS_TO_NS(3000));
		KeReleaseSemaphore(&sem);
	}
}

void task2(void *c)
{
	while(1)
	{
		if(true == KeAcquireSemaphoreWithTimeout(&sem, MS_TO_NS(780)))
		{
			PRINT("2");
			KeReleaseSemaphore(&sem);
		}
		else
			PRINT("0");
		// KeAcquireMutex(&s);
		// CmPrintf("2");
		// KeReleaseMutex(&s);
		//KeTaskYield();
	}
}

NORETURN static void KeInit(void)
{	
	//initialize core kernel modules
	//these function do not return any values, but will panic on any failure
	MmGdtInit();
	MmGdtApplyFlat();
	MmInitPhysicalAllocator(&kernelArgs);
	MmInitVirtualAllocator();
	MmInitializeMemoryDescriptorAllocator();
	MmInitDynamicMemory(&kernelArgs);
	//boot VGA driver can be initialized when dynamic memory allocator is available
	if(OK != BootVgaInit())
		ItHardReset();
	
	if(OK != HalInit())
		KePanicEx(BOOT_FAILURE, HAL_INIT_FAILURE, 0, 0, 0);

	KePrepareTSS(0);

	ItInit(); //initialize interrupts and exceptions
	IoVfsInit();
	IoFsInit();
	
	if(0 == kernelArgs.initrdSize)
	{
		PRINT("Initial ramdisk not found. Unable to boot.\n");
		while(1)
			;
	}

	STATUS ret = OK;
	if(OK != (ret = InitrdInit(kernelArgs.initrdAddress)))
	{
		PRINT("Initial ramdisk initialization error %d. Unable to boot.\n", (int)ret);
		while(1)
			;
	}

	if(OK != ExLoadKernelSymbols("/initrd/kernel32.elf"))
	{
		PRINT("Kernel symbol loading failed. Unable to boot.\n");
		while(1)
			;
	}

	KeSchedulerStart();
	

	if(OK != IoInitDeviceManager("ACPI"))
	{
		PRINT("FAILURE");
	}

	static struct KeTaskControlBlock *t1, *t2;

	KeCreateProcessRaw("pr1", NULL, PL_KERNEL, task1, NULL, &t1);
	KeCreateProcessRaw("pr2", NULL, PL_KERNEL, task2, NULL, &t2);
	KeEnableTask(t1);
	KeEnableTask(t2);

	while(1)
	{
		KeTaskYield();
	}
}

NORETURN void KeEntry(struct KernelEntryArgs args)
{
	//store kernel arguments locally as the previous stack will be destroyed
	CmMemcpy(&kernelArgs, &args, sizeof(args));
	
	uintptr_t kernelStackAddress = (uintptr_t)&_KERNEL_INITIAL_STACK_ADDRESS; //get temporary stack address
	ASM("mov esp, %0" : : "m" (kernelStackAddress) : ); //set up stack top address in ESP register
	ASM("mov ebp, %0" : : "m" (kernelStackAddress) : ); //set up base address in EBP register
	//do not declare any local variables here
	//stack and base pointers were changed and the compiler is not aware of this fact
	//just jump to the initialization routine
	KeInit();
}