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
#include "rtl/stdlib.h"
#include "ddk/tty.h"
#include "hal/debug.h"
#include "hal/arch.h"



static void KeStartInit(void)
{
	STATUS status = OK;
	struct KeTaskControlBlock *tcb = nullptr;
	const char *path = nullptr;
	const char **argv = nullptr;

	if(!ConfigGetKernelParam("init", &path) || (nullptr == path))
	{
		path = DEFAULT_INIT_PATH;
	}

	const char *envp[] = {nullptr};

	ConfigGetUserParams(&argv);
	argv[0] = path;

	status = KeCreateUserProcess(path, KE_TASK_FLAG_CRITICAL, argv, envp, NULL, &tcb);
	if(OK != status)
		KePanic(NO_WORKING_INIT);

	status = KeEnableTask(tcb);
	if(OK != status)
		KePanic(NO_WORKING_INIT);
}

static void KeInitProcess(void *context)
{
	STATUS ret = OK;

	HalInitPhase4();

	if(OK != ExLoadKernelSymbols(context))
		FAIL_BOOT("unable to load kernel symbols\n");

	if(OK != (ret = IoInitrdInit(context)))
		FAIL_BOOT("unable to find initial ramdisk\n");

	if(OK != (ret = IoInitrdMount(INITRD_MOUNT_POINT)))
		FAIL_BOOT("unable to mount initial ramdisk\n");

	if(OK != (ret = ExInitializeDriverManager()))
		FAIL_BOOT("unable to initialize driver manager\n");

	if(OK != IoInitDeviceManager(context, HAL_ROOT_DEVICE_ID))
		FAIL_BOOT("unable to initialize ACPI subsystem\n");

	LOG(SYSLOG_INFO, "Waiting for the main file system to be mounted...\n");
	IoWaitForMainFileSystemMount(KE_MUTEX_NO_TIMEOUT);

	if(OK != ExUpdateDriverDatabasePath())
		FAIL_BOOT("unable to update driver database path");

	IoRetryBuildDeviceStackAndEnumerate();
	
	if(OK != ExLoadKernelDriversByName("null.ndb", NULL, NULL))
		FAIL_BOOT("unable to load null device driver");

	if(OK != ExLoadKernelDriversByName("tty.ndb", NULL, NULL))
		FAIL_BOOT("unable to load TTY device driver");

	KeStartInit();
}

/**
 * @brief Kernel entry point
 * @param *arg Bootloader-specific data pointer
 * @attention This function never returns
 * 
 * This function is called by the architecture-specific bootstrap code.
 * Some bootloaders, such as Multiboot2, provide data to the kernel.
 * The passed parameter \a arg points to this data. \a arg must point
 * to a mapped kernel memory space, that is, all data must reside in kernel memory.
 */
[[noreturn]] void KeEntry(void *arg)
{	
	RtlDetectEndianness();

	//initialize core kernel modules
	//these function do not return any values, but will panic on any failure
	MmInitPhysicalAllocator(arg);

	HalCallConstructors();

	HalInitPhase1();

	MmInitializeMemoryDescriptorAllocator();
	MmInitDynamicMemory();

	HalInitPhase2();

	ItInit();

	HalInitPhase3();

	ConfigParseKernelArguments(arg);

	HalDebugPortInit();

	LOG(SYSLOG_INFO, KERNEL_FULL_NAME_STRING);
	LOG(SYSLOG_INFO, "Booting...");

	ObInitialize();
	RtlInitializeRandom();

	IoVfsInit();
	IoFsInit();
	KeDpcInitialize();

	KeStartScheduler(KeInitProcess, arg);

	//never reached
	while(1)
		;
}