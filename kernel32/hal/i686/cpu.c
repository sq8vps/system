#include "cpu.h"
#include "lapic.h"
#include "lapic.h"
#include "hal/mm.h"
#include "ke/sched/sleep.h"
#include "ke/core/panic.h"
#include "config.h"
#include "memory.h"
#include "hal/arch.h"
#include "gdt.h"
#include "hal/cpu.h"
#include "interrupts/it.h"
#include "math.h"
#include "lapic.h"
#include "ke/sched/sched.h"
#include "ke/sched/idle.h"
#include "ipi.h"
#include "mm/palloc.h"
#include "rtl/string.h"
#include "syscall.h"

#ifdef SMP

extern void I686StartAp(void);
extern void I686StartApEnd(void);

static struct
{
    uintptr_t cpuId;
    uintptr_t lapicId;
    uintptr_t cr3;
    uintptr_t esp;
    uintptr_t eip;
} PACKED I686StartApData[MAX_CPU_COUNT];
static volatile uint32_t I686StartedCpuCount = 1;
static volatile uint32_t I686ReadyCpuCount = 1;
static volatile uint8_t I686ApCanContinue = 0;

void I686CpuBootstrap(uint32_t cpuId);

#define I686_AP_BOOTSTRAP_ADDRESS 0x1000
#define I686_AP_BOOTSTRAP_DATA_ADDRESS 0x2000

void HalHaltAllCpus(void)
{
#ifdef SMP
    I686SendShutdownCpus();
#endif
}

STATUS I686ConfigureBootstrapCpu(void)
{
    for(uint32_t i = 0; i < HalGetCpuCount(); i++)
    {
        struct HalCpu *cpu = HalGetCpuEntry(i);
        if(cpu->extensions.bootstrap)
        {
            I686InstallIdt(cpu->number);
            if(OK != GdtAddCpu(cpu->number))
                FAIL_BOOT("failed to add TSS for CPU");
            GdtLoadTss(cpu->number);
            I686InitializeSyscall();
            return OK;
        }
    }   

    FAIL_BOOT("no bootstrap CPU. Broken ACPI/MP tables?")

    return OK;
}

STATUS I686StartProcessors(void)
{
    STATUS status = OK;

    uint32_t CpuCount = HalGetCpuCount();
    if(CpuCount <= 1)
        return OK;

    void *stack = (void*)HAL_KERNEL_SPACE_BASE;

    status = HalMapMemoryEx(I686_AP_BOOTSTRAP_ADDRESS, I686_AP_BOOTSTRAP_ADDRESS, 
        ALIGN_UP(I686_AP_BOOTSTRAP_DATA_ADDRESS + sizeof(I686StartApData) - I686_AP_BOOTSTRAP_ADDRESS, PAGE_SIZE), 
        MM_FLAG_WRITABLE | MM_FLAG_CACHE_DISABLE | MM_FLAG_WRITE_THROUGH);
    if(OK != status)
        FAIL_BOOT("cannot map memory for CPU bootstrap");

    status = MmAllocateMemory((uintptr_t)stack - (CpuCount - 1) * PAGE_SIZE,
        (CpuCount - 1) * PAGE_SIZE, MM_FLAG_WRITABLE);
    if(OK != status)
        FAIL_BOOT("cannot allocate temporary stacks from CPU boostrap")

    uint32_t i = 0, k = 0;
    for(i = 0; i < CpuCount; i++)
    {
        struct HalCpu *cpu = HalGetCpuEntry(i);
        if(NULL == cpu)
            continue;
        if(cpu->extensions.bootstrap)
            continue;
        I686StartApData[k].cpuId = cpu->number;
        I686StartApData[k].cr3 = I686GetPageDirectoryAddress();
        I686StartApData[k].esp = (uintptr_t)stack - (k * PAGE_SIZE);
        I686StartApData[k].lapicId = cpu->extensions.lapicId;
        I686StartApData[k].eip = (uintptr_t)I686CpuBootstrap;
        k++;
    }
    I686StartApData[k].esp = 0;

    RtlMemcpy((void*)I686_AP_BOOTSTRAP_ADDRESS, I686StartAp, (uintptr_t)I686StartApEnd - (uintptr_t)I686StartAp);
    RtlMemcpy((void*)I686_AP_BOOTSTRAP_DATA_ADDRESS, I686StartApData, sizeof(I686StartApData));

    for(uint16_t i = 0; i < CpuCount; i++)
    {
        struct HalCpu *cpu = HalGetCpuEntry(i);
        if(NULL == cpu)
            continue;
        if(cpu->extensions.bootstrap || !cpu->usable)
            continue;
        
        ApicSendIpi(APIC_IPI_DESTINATION_NORMAL, cpu->extensions.lapicId, APIC_IPI_INIT, 0, true);
        ApicWaitForIpiDelivery(US_TO_NS(200));
        ApicSendIpi(APIC_IPI_DESTINATION_NORMAL, cpu->extensions.lapicId, APIC_IPI_INIT, 0, false);
        ApicWaitForIpiDelivery(US_TO_NS(200));
    }
    KeDelay(MS_TO_NS(10));
    for(uint16_t i = 0; i < CpuCount; i++)
    {
        struct HalCpu *cpu = HalGetCpuEntry(i);
        if(NULL == cpu)
            continue;
        if(cpu->extensions.bootstrap || !cpu->usable)
            continue;
        
        uint32_t lastCpuCount = __atomic_load_n(&I686StartedCpuCount, __ATOMIC_SEQ_CST);
        //first wait for IPI delivery up to a given time
        //if that passes, then wait a bit and check if the AP woke up
        //if any of these failed, then repeat
        ApicSendIpi(APIC_IPI_DESTINATION_NORMAL, cpu->extensions.lapicId, APIC_IPI_START_UP, 0x01, true);
        if(OK == ApicWaitForIpiDelivery(US_TO_NS(200)))
        {
            KeDelay(US_TO_NS(200)); 
            //counter changed, continue with next AP
            if(lastCpuCount != __atomic_load_n(&I686StartedCpuCount, __ATOMIC_SEQ_CST))
                continue;
        }
        ApicSendIpi(APIC_IPI_DESTINATION_NORMAL, cpu->extensions.lapicId, APIC_IPI_START_UP, 0x01, true);
        if(OK == ApicWaitForIpiDelivery(US_TO_NS(200)))
        {
            //this time wait much longer
            KeDelay(MS_TO_NS(1000)); 
            if(lastCpuCount != __atomic_load_n(&I686StartedCpuCount, __ATOMIC_SEQ_CST))
                continue;
        }
        //second attempt failed, skip AP
    }

    while(__atomic_load_n(&I686ReadyCpuCount, __ATOMIC_SEQ_CST) != I686StartedCpuCount)
        TIGHT_LOOP_HINT();

    if(OK != I686InitializeIpi())
        FAIL_BOOT("IPI module initialization failed");

    __atomic_store_n(&I686ApCanContinue, 1, __ATOMIC_SEQ_CST);

    status = HalUnmapMemoryEx(I686_AP_BOOTSTRAP_ADDRESS,
        ALIGN_UP(I686_AP_BOOTSTRAP_DATA_ADDRESS + sizeof(I686StartApData) - I686_AP_BOOTSTRAP_ADDRESS, PAGE_SIZE));
    if(OK != status)
        FAIL_BOOT("cannot unmap memory after CPU bootstrap")

    return OK;
}

void I686CpuBootstrap(uint32_t cpuId)
{
    __atomic_fetch_add(&I686StartedCpuCount, 1, __ATOMIC_SEQ_CST);
    GdtApply();
    GdtAddCpu(cpuId);
    GdtLoadTss(cpuId);
    I686InstallIdt(cpuId);
    I686InitMath();
    ApicInitAp();
    I686InitializeSyscall();

    __atomic_fetch_add(&I686ReadyCpuCount, 1, __ATOMIC_SEQ_CST);

    while(0 == __atomic_load_n(&I686ApCanContinue, __ATOMIC_SEQ_CST))
        TIGHT_LOOP_HINT();

    KeJoinScheduler();
    
    while(1)
    {
        KeTaskYield();
    }
}

uint16_t HalGetCurrentCpu(void)
{
    register uint16_t t;
    //get GDT descriptor with TSS from task register
    ASM("str %0" : "=r" (t) :);
    return GDT_CPU(GDT_ENTRY(t));
}

#endif