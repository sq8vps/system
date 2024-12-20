#include <stdint.h>
#include "it/it.h"
#include "mm/palloc.h"
#include "mm/heap.h"
#include "mm/mm.h"
#include "ex/exec.h"
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
#include "rtl/order.h"
#include "io/dev/vol.h"
#include "ddk/fs.h"
#include "hal/arch.h"
#include "multiboot.h"
#include "rtl/stdlib.h"

KeMutex s = KeMutexInitializer;
KeSemaphore sem = KeSemaphoreInitializer;

void task1(void *c)
{
	UNUSED(c);
	while(1)
	{
		KeAcquireSemaphore(&sem, 1);
		//PRINT("1");
		KePutTaskToSleep(KeGetCurrentTask(), MS_TO_NS(3000));
		KeReleaseSemaphore(&sem, 1);
	}
}

void task2(void *c)
{
	UNUSED(c);
	while(1)
	{
		if(true == KeAcquireSemaphoreEx(&sem, 1, MS_TO_NS(780)))
		{
			//PRINT("2");
			KeReleaseSemaphore(&sem, 1);
		}
		//else
			//PRINT("0");
		// KeAcquireMutex(&s);
		// RtlPrintf("2");
		// KeReleaseMutex(&s);
		//KeTaskYield();
	}
}

struct KeTaskControlBlock *t1, *t2;

static void KeInitProcess(void *context)
{
	STATUS ret = OK;

	if(OK != ExLoadKernelSymbols(context))
		FAIL_BOOT("unable to load kernel symbols\n");

	if(OK != (ret = IoInitrdInit(context)))
		FAIL_BOOT("unable to find initial ramdisk\n");

	if(OK != (ret = IoInitrdMount(INITRD_MOUNT_POINT)))
		FAIL_BOOT("unable to mount initial ramdisk\n");

	if(OK != (ret = ExInitializeDriverManager()))
		FAIL_BOOT("unable to initialize driver manager\n");

	if(OK != IoInitDeviceManager("ACPI"))
		FAIL_BOOT("unable to initialize ACPI subsystem\n");

	LOG(SYSLOG_INFO, "Waiting for the main file system to be mounted...\n");
	if(!IoWaitForMainFileSystemMount(MS_TO_NS(20000)))
		FAIL_BOOT("main file system not mounted within the given time limit");

	if(OK != ExUpdateDriverDatabasePath())
		FAIL_BOOT("unable to update driver database path");
	
	if(OK != ExLoadKernelDriversByName("null.ndb", NULL, NULL))
		FAIL_BOOT("unable to load null device driver");
	
	int fd[3] = {0, 1, 2};
	IoOpenFile("/dev/null", IO_FILE_WRITE, IO_FILE_FLAG_SHARED | IO_FILE_FLAG_FORCE_HANDLE_NUMBER, &fd[0]);
	IoOpenFile("/dev/null", IO_FILE_WRITE, IO_FILE_FLAG_SHARED | IO_FILE_FLAG_FORCE_HANDLE_NUMBER, &fd[1]);
	IoOpenFile("/dev/null", IO_FILE_WRITE, IO_FILE_FLAG_SHARED | IO_FILE_FLAG_FORCE_HANDLE_NUMBER, &fd[2]);
	
	IoVfsCreateLink("/dev/stdin", "/task/self/fd/0", 0);
	IoVfsCreateLink("/dev/stdout", "/task/self/fd/1", 0);
	IoVfsCreateLink("/dev/stderr", "/task/self/fd/2", 0);

	struct KeTaskControlBlock *tcb;
	const char *argv[] = {"test.elf", "-a", "12345", NULL};
	const char *envp[] = {"PATH=/", "NABLA", NULL};
	struct KeTaskFileMapping map[4] = {{.mapFrom = 0, .mapTo = 0}, {.mapFrom = 1, .mapTo = 1}, {.mapFrom = 2, .mapTo = 2}, KE_TASK_FILE_MAPPING_END};

	KeCreateUserProcess("/main/system/test", 0, argv, envp, map, &tcb);
	KeEnableTask(tcb);

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
	RtlDetectEndianness();

	//initialize core kernel modules
	//these function do not return any values, but will panic on any failure
	MmInitPhysicalAllocator(mb2h);

	HalCallConstructors();

	HalInitPhase1();

	MmInitializeMemoryDescriptorAllocator();
	MmInitDynamicMemory();

	HalInitPhase2();

	LOG(SYSLOG_INFO, KERNEL_FULL_NAME_STRING);
	LOG(SYSLOG_INFO, "Booting...");

	ItInit(); //initialize interrupts and exceptions

	HalInitPhase3();

	ObInitialize();
	RtlInitializeRandom();

	IoVfsInit();
	IoFsInit();
	KeDpcInitialize();

	KeStartScheduler(KeInitProcess, mb2h);

	//never reached
	while(1)
		;
}