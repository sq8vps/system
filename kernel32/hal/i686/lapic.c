#if defined(__i686__) || defined(__amd64__)

#include "lapic.h"
#include "dcpuid.h"
#include "mm/mmio.h"
#include "it/it.h"
#include "ioapic.h"
#include "pit.h"
#include "msr.h"
#include "tsc.h"
#include "hal/time.h"
#include "hal/arch.h"
#include "config.h"
#include "hal/cpu.h"
#include "ipi.h"

#include "ke/core/panic.h"

#define LAPIC_SPURIOUS_VECTOR 255

enum ApicTimerDivider
{
    APIC_TIMER_DIVIDE_2 = 0,
    APIC_TIMER_DIVIDE_4 = 1,
    APIC_TIMER_DIVIDE_8 = 2,
    APIC_TIMER_DIVIDE_16 = 3,
    APIC_TIMER_DIVIDE_32 = 8,
    APIC_TIMER_DIVIDE_64 = 9,
    APIC_TIMER_DIVIDE_128 = 10,
    APIC_TIMER_DIVIDE_1 = 11,
};

#define LAPIC_ID_OFFSET 0x20
#define LAPIC_VERSION_OFFSET 0x30
#define LAPIC_TPR_OFFSET 0x80
#define LAPIC_APR_OFFSET 0x90
#define LAPIC_PPR_OFFSET 0xA0
#define LAPIC_EOI_OFFSET 0xB0
#define LAPIC_RRD_OFFSET 0xC0
#define LAPIC_LOGICAL_DESTINATION_OFFSET 0xD0
#define LAPIC_DESTINATION_FORMAT_OFFSET 0xE0
#define LAPIC_SPURIOUS_INTERRUPT_OFFSET 0xF0
#define LAPIC_ISR_OFFSET 0x100
#define LAPIC_TMR_OFFSET 0x180
#define LAPIC_IRR_OFFSET 0x200
#define LAPIC_ERROR_STATUS_OFFSET 0x280
#define LAPIC_LVT_CMCI_OFFSET 0x2F0
#define LAPIC_ICR_OFFSET 0x300
#define LAPIC_LVT_TIMER_OFFSET 0x320
#define LAPIC_LVT_THERMAL_SENSOR_OFFSET 0x330
#define LAPIC_LVT_PERF_MONITORING_OFFSET 0x340
#define LAPIC_LINT0_OFFSET 0x350
#define LAPIC_LINT1_OFFSET 0x360
#define LAPIC_LVT_ERROR_OFFSET 0x370
#define LAPIC_TIMER_INITIAL_COUNT_OFFSET 0x380
#define LAPIC_TIMER_CURRENT_COUNT_OFFSET 0x390
#define LAPIC_TIMER_DIVIDER_OFFSET 0x3E0

#define LAPIC_ICR_RESERVED_MASK 0xFFF32000
#define LAPIC_ICR_LEVEL_ASSERT (1 << 14)
#define LAPIC_ICR_LEVEL_DEASSERT (0)
#define LAPIC_ICR_TRIGGER_LEVEL (1 << 15)
#define LAPIC_ICR_TRIGGER_EDGE (0)

#define LAPIC_ICR_DELIVERY_STATUS_BIT (1 << 12)

#define LAPIC_LOCAL_MASK (1 << 16)

#define LAPIC_MODE_FIXED (0 << 8)
#define LAPIC_MODE_LOWEST_PRIORITY (1 << 8)
#define LAPIC_MODE_SMI (2 << 8)
#define LAPIC_MODE_NMI (4 << 8)
#define LAPIC_MODE_INIT (5 << 8)
#define LAPIC_MODE_START_UP (6 << 8)
#define LAPIC_MODE_EXTINT (7 << 8)

#define LAPIC_DELIVERY_STATUS_MASK (1 << 12)

#define LAPIC_POLARITY_ACTIVE_LOW (1 << 13)
#define LAPIC_POLARITY_ACTIVE_HIGH (0 << 13)
#define LAPIC_TIMER_PERIODIC_FLAG (1 << 17)
#define LAPIC_TIMER_ONE_SHOT_FLAG (0 << 17)
#define LAPIC_TIMER_TSC_DEADLINE_FLAG (2 << 17)

#define LAPIC_VECTOR_MASK 0xFF

/**
 * @brief Check if TSC is available
 * @return True if TSC is available, false otherwise
 */
#define CHECK_TSC_AVAILABLE() CpuidCheckIfTscAvailable()

/**
 * @brief Check if TSC is available and reliable and can be used as a system timer
 * @return True if TSC is available and reliable, false otherwise
 */
#define CHECK_TSC_USABLE() (CpuidCheckIfTscAvailable() && CpuidCheckIfTscInvariant() && CpuidCheckIfTscDeadlineAvailable())

#define LAPIC_DEFAULT_TIMER_DIVIDER APIC_TIMER_DIVIDE_16

static struct
{
    volatile uint8_t *space; /**< Mapped LAPIC space */
    bool tscAvailable; /**< TSC is available */
    bool useTsc; /**< Use TSC = TSC is available and reliable */
} ApicState = {.space = NULL, .useTsc = false};

#ifdef SMP
static uint64_t ApicCounter[MAX_CPU_COUNT];
#else
static uint64_t ApicCounter = 0;
#endif

static uint64_t ApicTimerGetRaw(void *context);

static struct HalClockSource ApicClockSource = 
{
    .name = "APIC",
    .rating = 200,
    .read = ApicTimerGetRaw,
    .context = NULL
};

/**
 * @brief A convenience macro for 32-bit Local APIC register access
*/
#define LAPIC(register) (*(volatile uint32_t*)(ApicState.space + (register)))

static STATUS ApicSpuriousInterruptHandler(void *context)
{
    KePanic(UNEXPECTED_FAULT);
    UNUSED(context);
    return OK;
}

STATUS ApicSendEoi(void)
{
    if(unlikely(NULL == ApicState.space))
        return DEVICE_NOT_AVAILABLE;

    LAPIC(LAPIC_EOI_OFFSET) = 0;
    
    return OK;
}

void ApicSendIpi(enum ApicIpiDestination shorthand, uint8_t destination, enum ApicIpiMode mode, uint8_t vector, bool assert)
{
    if(APIC_IPI_DESTINATION_NORMAL == shorthand)
        LAPIC(LAPIC_ICR_OFFSET + 0x10) = (LAPIC(LAPIC_ICR_OFFSET + 0x10) & 0x00FFFFFF) | ((uint32_t)destination << 24);
    LAPIC(LAPIC_ICR_OFFSET) = (LAPIC(LAPIC_ICR_OFFSET) & LAPIC_ICR_RESERVED_MASK) 
        | ((shorthand & 0x3) << 18) | ((mode & 0x7) << 8) | ((mode == APIC_IPI_INIT) ? LAPIC_ICR_TRIGGER_LEVEL : LAPIC_ICR_TRIGGER_EDGE) 
        | ((assert || (mode != APIC_IPI_INIT)) ? LAPIC_ICR_LEVEL_ASSERT : LAPIC_ICR_LEVEL_DEASSERT) | vector;
}

STATUS ApicWaitForIpiDelivery(uint64_t timeLimit)
{
    uint64_t end = HalGetTimestamp() + timeLimit;
    while(LAPIC(LAPIC_ICR_OFFSET) & LAPIC_ICR_DELIVERY_STATUS_BIT)
    {
        if(end <= HalGetTimestamp())
            return TIMEOUT;
        TIGHT_LOOP_HINT();
    }
    return OK;
}

STATUS ApicInitAp(void)
{
    if(NULL == ApicState.space)
        return DEVICE_NOT_AVAILABLE;

    MsrSet(MSR_IA32_APIC_BASE, MsrGet(MSR_IA32_APIC_BASE) | MSR_IA32_APIC_BASE_ENABLE_MASK);

    LAPIC(LAPIC_LINT0_OFFSET) = LAPIC_LOCAL_MASK;
    LAPIC(LAPIC_LINT1_OFFSET) = LAPIC_LOCAL_MASK;

    LAPIC(LAPIC_TPR_OFFSET) = 0;
    LAPIC(LAPIC_DESTINATION_FORMAT_OFFSET) = 0xFFFFFFFF;

    LAPIC(LAPIC_SPURIOUS_INTERRUPT_OFFSET) = 0x100 | LAPIC_SPURIOUS_VECTOR;

    return OK;
}

static void ApicTimerMeasurementCallback(bool finished, void *context)
{
    UNUSED(context);
    if(!finished)
    {
        LAPIC(LAPIC_TIMER_INITIAL_COUNT_OFFSET) = 0xFFFFFFFF;
    }
    else
    {
        uint32_t value = LAPIC(LAPIC_TIMER_CURRENT_COUNT_OFFSET);
        ApicClockSource.frequency = ((uint64_t)100) * ((uint64_t)(0xFFFFFFFF - value));
    }
}

STATUS ApicInitBsp(void)
{
    STATUS ret = OK;

    ApicState.tscAvailable = CHECK_TSC_AVAILABLE();
    ApicState.useTsc = CHECK_TSC_USABLE();

    if(OK != (ret = ApicInitAp()))
    {
        ApicState.space = NULL;
        return ret;
    }

    if(OK != (ret = ItInstallInterruptHandler(LAPIC_SPURIOUS_VECTOR, ApicSpuriousInterruptHandler, NULL)))
    {
        ApicState.space = NULL;
        return ret;
    }

    if(OK != (ret = ItSetInterruptHandlerEnable(LAPIC_SPURIOUS_VECTOR, ApicSpuriousInterruptHandler, true)))
    {
        ItUninstallInterruptHandler(LAPIC_SPURIOUS_VECTOR, ApicSpuriousInterruptHandler);
        ApicState.space = NULL;
        return ret;
    }

    LAPIC(LAPIC_LINT0_OFFSET) = LAPIC_MODE_EXTINT | LAPIC_ICR_TRIGGER_EDGE | LAPIC_POLARITY_ACTIVE_HIGH;

    LAPIC(LAPIC_TIMER_DIVIDER_OFFSET) = LAPIC_DEFAULT_TIMER_DIVIDER & 0b1011;
    PitDoSingleShot(10000, ApicTimerMeasurementCallback, ApicTimerMeasurementCallback, NULL);
    
    return HalRegisterClockSource(&ApicClockSource);
}

STATUS ApicInit(uintptr_t address)
{
    if(0 == address)
        return BAD_PARAMETER;

    ApicState.space = MmMapMmIo(address, PAGE_SIZE);
    if(NULL == ApicState.space)
        return OUT_OF_RESOURCES;
    return OK;
}

STATUS ApicConfigureSystemTimer(uint8_t vector)
{
    if(vector < IT_FIRST_INTERRUPT_VECTOR)  
        return BAD_PARAMETER;
    
    //if the bootstrap CPU can use TSC, but the current CPU cannot, there system is incompatible
    //if the bootstrap CPU cannot use TSC, then stick to the LAPIC timer regardless of the current CPU capabilities
    if(ApicState.useTsc && !CHECK_TSC_USABLE())
        return NOT_SUPPORTED;

    if(NULL != ApicState.space)
    {
        LAPIC(LAPIC_TIMER_DIVIDER_OFFSET) = LAPIC_DEFAULT_TIMER_DIVIDER & 0b1011;
        LAPIC(LAPIC_LVT_TIMER_OFFSET) = vector;
        if(ApicState.useTsc)
            LAPIC(LAPIC_LVT_TIMER_OFFSET) |= LAPIC_TIMER_TSC_DEADLINE_FLAG;
        else
            LAPIC(LAPIC_LVT_TIMER_OFFSET) |= LAPIC_TIMER_ONE_SHOT_FLAG;
        
        return OK;
    }

    return DEVICE_NOT_AVAILABLE;   
}

void ApicStartSystemTimer(uint64_t time)
{
    if(ApicState.useTsc)
    {
        MsrSet(MSR_IA32_TSC_DEADLINE, TscCalculateRaw(time * (uint64_t)1000) + TscGetRaw(NULL));
    }
    else
    {
        ATOMIC_ADD_FETCH(
#ifndef SMP
            &ApicCounter,
#else
            &ApicCounter[HalGetCurrentCpu()], 
#endif
            (uint64_t)LAPIC(LAPIC_TIMER_INITIAL_COUNT_OFFSET) - (uint64_t)LAPIC(LAPIC_TIMER_CURRENT_COUNT_OFFSET), ATOMIC_ACQ_REL);
        LAPIC(LAPIC_TIMER_INITIAL_COUNT_OFFSET) = (time * ApicClockSource.frequency) / (uint64_t)1000000;
    }
    LAPIC(LAPIC_LVT_TIMER_OFFSET) &= ~LAPIC_LOCAL_MASK;

    if(ApicState.tscAvailable)
        TscUpdate();
}

static uint64_t ApicTimerGetRaw(void *context)
{
    UNUSED(context);
    
    if(ApicState.useTsc)
        return TscGetRaw(NULL);
    else
        return ApicCounter[HalGetCurrentCpu()] 
            + (uint64_t)LAPIC(LAPIC_TIMER_INITIAL_COUNT_OFFSET) - (uint64_t)LAPIC(LAPIC_TIMER_CURRENT_COUNT_OFFSET);
}

static int ApicSynchronize(void *context)
{
    uint32_t cpu = HalGetCurrentCpu();
    const int64_t delta = *((uint64_t*)context) - ApicTimerGetRaw(NULL);

    ApicCounter[cpu] += delta;

    return 0;
}

void ApicSynchronizeTimers(void)
{
    HalCpuBitmap cpu = HAL_CPU_ALL;
    int results[MAX_CPU_COUNT];
    uint64_t current = ApicTimerGetRaw(NULL);

    I686InvokeRemoteFunction(&cpu, ApicSynchronize, &current, results);
}

STATUS HalConfigureSystemTimer(uint8_t vector)
{   
    return ApicConfigureSystemTimer(vector);
}

STATUS HalStartSystemTimer(uint64_t time)
{
    ApicStartSystemTimer(time);
    return OK;
}

STATUS ApicSetTaskPriority(uint8_t priority)
{
    if(unlikely(NULL == ApicState.space))
        return DEVICE_NOT_AVAILABLE;
    LAPIC(LAPIC_TPR_OFFSET) = (priority & 0xF) << 4;
    return OK;
}

uint8_t ApicGetTaskPriority(void)
{
    if(unlikely(NULL == ApicState.space))
        return 0;
    return (LAPIC(LAPIC_TPR_OFFSET) >> 4) & 0xF;
}

uint8_t ApicGetProcessorPriority(void)
{
    if(unlikely(NULL == ApicState.space))
        return 0;
    return (LAPIC(LAPIC_PPR_OFFSET) >> 4) & 0xF;
}

uint8_t ApicGetCurrentId(void)
{
    return (LAPIC(LAPIC_ID_OFFSET) >> 24);
}

#endif