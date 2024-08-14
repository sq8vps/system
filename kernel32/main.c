#include <stdint.h>
#include "it/it.h"
#include "mm/palloc.h"
#include "mm/heap.h"
#include "mm/mm.h"
#include "../cdefines.h"
#include "ex/exec.h"
#include "common.h"
#include "ex/kdrv/kdrv.h"
#include "mm/dynmap.h"
#include "hal/hal.h"
#include "ke/task/task.h"
#include "ke/sched/sched.h"
#include "ke/core/mutex.h"
#include "hal/i686/bootvga/bootvga.h"
#include "ke/core/panic.h"
#include "io/fs/vfs.h"
#include "io/fs/fs.h"
#include "hal/interrupt.h"
#include "sdrv/initrd/initrd.h"
#include "ex/ksym.h"
#include "defines.h"
#include "ke/task/task.h"
#include "ke/sched/sched.h"
#include "ke/sched/sleep.h"
#include "io/dev/dev.h"
#include "ke/core/dpc.h"
#include "common/order.h"
#include "io/dev/vol.h"
#include "ddk/fs.h"
#include "hal/arch.h"

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
	CmDetectEndianness();
	//initialize core kernel modules
	//these function do not return any values, but will panic on any failure
	MmInitPhysicalAllocator(&kernelArgs);

	HalInitPhase1();

	MmInitializeMemoryDescriptorAllocator();
	MmInitDynamicMemory(&kernelArgs);

	HalInitPhase2();

	ItInit(); //initialize interrupts and exceptions

	HalInitPhase3();

	IoVfsInit();
	IoFsInit();
	KeDpcInitialize();
	
	if(0 == kernelArgs.initrdSize)
	{
		PRINT("Initial ramdisk not found. Unable to boot.\n");
		while(1)
			;
	}

	KeStartScheduler();

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
	

	if(OK != IoInitDeviceManager("ACPI"))
	{
		PRINT("FAILURE");
	}

	static struct KeTaskControlBlock *t1, *t2;

	KeCreateProcessRaw("pr1", NULL, PL_KERNEL, task1, NULL, &t1);
	KeCreateProcessRaw("pr2", NULL, PL_KERNEL, task2, NULL, &t2);
	KeEnableTask(t1);
	KeEnableTask(t2);

	KeSleep(MS_TO_NS(4000));
	IoMountVolume("/dev/hda0", "disk1");

	// struct IoFileHandle *h;
	// IoOpenKernelFile("/disk1/system/abc.cfg", IO_FILE_APPEND, 0, &h);
	// char *t = MmAllocateKernelHeapAligned(8192, 512);
	// CmMemset(t, 'A', 8142);
	// CmStrcpy(&t[8142], "Test dopisywania z przekroczeniem granicy klastra");
	// uint64_t actualSize = 0;
	// IoWriteKernelFileSync(h, t, 8192, 0, &actualSize);
	// asm("nop");

	while(1)
	{
		//KeTaskYield();
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