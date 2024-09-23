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
#include "io/initrd.h"
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
#include "multiboot.h"

KeMutex s = KeMutexInitializer;
KeSemaphore sem = KeSemaphoreInitializer;

void task1(void *c)
{
	while(1)
	{
		KeAcquireSemaphore(&sem);
		//PRINT("1");
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
			//PRINT("2");
			KeReleaseSemaphore(&sem);
		}
		//else
			//PRINT("0");
		// KeAcquireMutex(&s);
		// CmPrintf("2");
		// KeReleaseMutex(&s);
		//KeTaskYield();
	}
}

struct KeTaskControlBlock *t1, *t2;

static void KeInitProcess(void *context)
{
	STATUS ret = OK;

	if(OK != ExLoadKernelSymbols(context))
	{
		PRINT("Kernel symbol loading failed. Unable to boot.\n");
		while(1)
			;
	}

	if(OK != (ret = IoInitrdInit(context)))
	{
		PRINT("Initial ramdisk initialization error %d. Unable to boot.\n", (int)ret);
		while(1)
			;
	}

	if(OK != (ret = IoInitrdMount(INITRD_MOUNT_POINT)))
		FAIL_BOOT("initial ramdisk mount failed");

	if(OK != IoInitDeviceManager("ACPI"))
	{
		PRINT("FAILURE");
	}

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
		KeTaskYield();
	}
}

/**
 * @brief Kernel entry point
 * 
 * This function is called by the architecture-specific bootstrap code.
 * It is required to provide a Multiboot2 information structures to start the kernel.
 * The Multiboot2 info structures should be copied to kernel memory space anyway,
 * so the \a *mb2h pointer is just for convenience.
 * 
 * @param *mb2h Multiboot2 header pointer
 * @attention This function never returns
 */
NORETURN void KeEntry(struct Multiboot2InfoHeader *mb2h)
{	
	CmDetectEndianness();

	//initialize core kernel modules
	//these function do not return any values, but will panic on any failure
	MmInitPhysicalAllocator(mb2h);

	HalInitPhase1();

	MmInitializeMemoryDescriptorAllocator();
	MmInitDynamicMemory();

	HalInitPhase2();

	ItInit(); //initialize interrupts and exceptions

	HalInitPhase3();

	IoVfsInit();
	IoFsInit();
	KeDpcInitialize();

	KeStartScheduler(KeInitProcess, mb2h);

	//never reached
	while(1)
		;
}