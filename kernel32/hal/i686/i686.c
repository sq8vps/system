#if defined(__i686__) || defined(__amd64__)

#include "hal/hal.h"
#include "msr.h"
#include "dcpuid.h"
#include "ke/core/panic.h"
#include "memory.h"
#include "bootvga/bootvga.h"
#include "math.h"
#include "root.h"
#include "irq.h"
#include "it/it.h"
#include "cpu.h"
#include "time.h"
#include "pic.h"
#include "it/it.h"
#include "interrupts/it.h"
#include "mm/palloc.h"

struct MmMemoryPool HalPhysicalPool[HAL_PHYSICAL_MEMORY_POOLS] = 
{
    //standard pool - non-real-mode memory (>=1 MiB)
    [MM_PHYSICAL_POOL_STANDARD] = {.base = 0x100000, .size = 0x100000000 - 0x100000},
    //real mode memory pool - (>=4 KiB to keep real mode IVT and <1 MiB) - ISA DMA, CPU bootstrap
    [I686_PHYSICAL_POOL_LOWER] = {.base = 0x1000, .size = 0x100000 - 0x1000},
};

void HalInitPhase1(void)
{
    if(!CpuidCheckIfAvailable())
        FAIL_BOOT("CPUID not available");
    
    CpuidInit();

    if(!CpuidCheckIfFpuAvailable())
        FAIL_BOOT("FPU not available");
    
    if(!CpuidCheckIfTscAvailable())
        FAIL_BOOT("TSC not available");
    
    MsrInit();

    I686InitVirtualAllocator();

    if(OK != I686InitIdt())
        FAIL_BOOT("IDT initialization failed");

    //mask all PIC IRQs, because they are mapped to a real-mode interrupt vector by the default,
    //and these vectors are the exception vectors in protected mode
    PicSetIrqMask(0xFFFF);
    //install IDT temporarily so we can handle exceptions properly
    I686InstallIdt(0);
}

void HalInitPhase2(void)
{
    //boot VGA driver can be initialized when dynamic memory allocator is available
	if(OK != BootVgaInit())
		FAIL_BOOT("boot VGA driver initialization failed");
	
    if(OK != I686InitMath())
        FAIL_BOOT("math coprocessor initialization failed");

    if(OK != I686InitRoot())
        FAIL_BOOT("ACPI/MP/LAPIC initialization failed");
    
    if(OK != I686ConfigureBootstrapCpu())
        FAIL_BOOT("bootstrap CPU configuration failed");

    if(OK != I686InitInterruptController())
        FAIL_BOOT("IRQ controller initialization failed")

    if(OK != I686InitTimeController())
        FAIL_BOOT("time controller initialization failed");
}

void HalInitPhase3(void)
{
    for(uint16_t i = (HAL_PRIORITY_LEVEL_IPI << 4); i <= IT_LAST_INTERRUPT_VECTOR; i++)
    {
        if(i != ItReserveVector(i))
            FAIL_BOOT("internal x86 vector reservation failed");
    }
    if(IT_SYSTEM_TIMER_VECTOR != ItReserveVector(IT_SYSTEM_TIMER_VECTOR))
        FAIL_BOOT("system timer vector reservation failed");
#ifdef SMP
    if(OK != I686StartProcessors())
        FAIL_BOOT("application CPU start failed");
#endif
}

#endif