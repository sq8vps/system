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

//LAPIC configuration space handler
//remains NULL when APIC is unavailable
//use uint8_t for easy well-defined pointer arithmetics
static volatile uint8_t *lapic = NULL;

#define LAPIC_DEFAULT_TIMER_DIVIDER APIC_TIMER_DIVIDE_16

#ifndef SMP
static uint64_t ApicCounter = 0;
#else
static uint64_t ApicCounter[MAX_CPU_COUNT] = {[0 ... MAX_CPU_COUNT - 1] = 0};
#endif

static uint64_t ApicFrequency;

/**
 * @brief A convenience macro for 32-bit Local APIC register access
*/
#define LAPIC(register) (*(volatile uint32_t*)(lapic + (register)))

static STATUS spuriousInterruptHandler(void *context)
{
    return OK;
}

STATUS ApicSendEoi(void)
{
    if(NULL == lapic)
        return APIC_LAPIC_NOT_AVAILABLE;

    LAPIC(LAPIC_EOI_OFFSET) = 0;
    
    return OK;
}

// STATUS ApicSetNMI(uint8_t lint, uint16_t mpflags)
// {
//     if(NULL == lapic)
//         return APIC_LAPIC_NOT_AVAILABLE;
    
//     if(0 == lint)
//         LAPIC(LAPIC_LINT0_OFFSET) = LAPIC_MODE_NMI | (((mpflags & 3) == 0) ? LAPIC_POLARITY_ACTIVE_HIGH : LAPIC_POLARITY_ACTIVE_LOW);
//     else if(1 == lint)
//         LAPIC(LAPIC_LINT1_OFFSET) = LAPIC_MODE_NMI | (((mpflags & 3) == 0) ? LAPIC_POLARITY_ACTIVE_HIGH : LAPIC_POLARITY_ACTIVE_LOW);
//     else
//         return APIC_LAPIC_BAD_LINT_NUMBER;
    
//     return OK;
// }

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
    if(NULL == lapic)
        return APIC_LAPIC_NOT_AVAILABLE;

    MsrSet(MSR_IA32_APIC_BASE, MsrGet(MSR_IA32_APIC_BASE) | MSR_IA32_APIC_BASE_ENABLE_MASK);

    LAPIC(LAPIC_LINT0_OFFSET) = LAPIC_LOCAL_MASK;
    LAPIC(LAPIC_LINT1_OFFSET) = LAPIC_LOCAL_MASK;

    LAPIC(LAPIC_TPR_OFFSET) = 0;
    LAPIC(LAPIC_DESTINATION_FORMAT_OFFSET) = 0xFFFFFFFF;

    LAPIC(LAPIC_SPURIOUS_INTERRUPT_OFFSET) = 0x100 | LAPIC_SPURIOUS_VECTOR;

    return OK;
}

STATUS ApicInitBsp(void)
{
    STATUS ret = OK;
    if(OK != (ret = ApicInitAp()))
    {
        lapic = NULL;
        return ret;
    }

    if(OK != (ret = ItInstallInterruptHandler(LAPIC_SPURIOUS_VECTOR, spuriousInterruptHandler, NULL)))
    {
        lapic = NULL;
        return ret;
    }

    if(OK != (ret = ItSetInterruptHandlerEnable(LAPIC_SPURIOUS_VECTOR, spuriousInterruptHandler, true)))
    {
        ItUninstallInterruptHandler(LAPIC_SPURIOUS_VECTOR, spuriousInterruptHandler);
        lapic = NULL;
        return ret;
    }

    LAPIC(LAPIC_TIMER_DIVIDER_OFFSET) = LAPIC_DEFAULT_TIMER_DIVIDER & 0b1011;
    PitOneShotInit(10000); //10000us=10ms
    PitOneShotStart();
    LAPIC(LAPIC_TIMER_INITIAL_COUNT_OFFSET) = 0xFFFFFFFF;
    PitOneShotWait();
    uint32_t value = LAPIC(LAPIC_TIMER_CURRENT_COUNT_OFFSET);
    ApicFrequency = ((uint64_t)100) * ((uint64_t)(0xFFFFFFFF - value));
    
    return OK;
}

STATUS ApicInit(uintptr_t address)
{
    if(0 == address)
        return APIC_LAPIC_NOT_AVAILABLE;

    lapic = MmMapMmIo(address, PAGE_SIZE);
    if(NULL == lapic)
        return OUT_OF_RESOURCES;
    return OK;
}

STATUS ApicConfigureSystemTimer(uint8_t vector)
{
    if(vector < IT_FIRST_INTERRUPT_VECTOR)  
        return BAD_INTERRUPT_VECTOR;
    
    if(lapic)
    {
        LAPIC(LAPIC_TIMER_DIVIDER_OFFSET) = LAPIC_DEFAULT_TIMER_DIVIDER & 0b1011;
        LAPIC(LAPIC_LVT_TIMER_OFFSET) = vector;
        if(CpuidCheckIfTscAvailable() && CpuidCheckIfTscInvariant() && CpuidCheckIfTscDeadlineAvailable())
            LAPIC(LAPIC_LVT_TIMER_OFFSET) |= LAPIC_TIMER_TSC_DEADLINE_FLAG;
        else
            LAPIC(LAPIC_LVT_TIMER_OFFSET) |= LAPIC_TIMER_ONE_SHOT_FLAG;
        
        return OK;
    }

    return APIC_LAPIC_NOT_AVAILABLE;   
}

void ApicStartSystemTimer(uint64_t time)
{
    if(LAPIC(LAPIC_LVT_TIMER_OFFSET) & LAPIC_TIMER_TSC_DEADLINE_FLAG)
    {
        MsrSet(MSR_IA32_TSC_DEADLINE, TscCalculateRaw(time * (uint64_t)1000) + TscGetRaw());
    }
    else
    {

        __atomic_add_fetch(
#ifndef SMP
            &ApicCounter,
#else
            &ApicCounter[HalGetCurrentCpu()], 
#endif
            (uint64_t)LAPIC(LAPIC_TIMER_INITIAL_COUNT_OFFSET) - (uint64_t)LAPIC(LAPIC_TIMER_CURRENT_COUNT_OFFSET), __ATOMIC_SEQ_CST);
        LAPIC(LAPIC_TIMER_INITIAL_COUNT_OFFSET) = time * ApicFrequency / (uint64_t)1000000;
    }
    LAPIC(LAPIC_LVT_TIMER_OFFSET) &= ~LAPIC_LOCAL_MASK;
}

#ifndef SMP
uint64_t ApicGetTimestamp(void)
{
    return (uint64_t)((1000000000. * ((double)__atomic_load_n(&ApicCounter, __ATOMIC_SEQ_CST)  + (double)LAPIC(LAPIC_TIMER_INITIAL_COUNT_OFFSET) 
        - (double)LAPIC(LAPIC_TIMER_CURRENT_COUNT_OFFSET))) / (double)ApicFrequency);
}

uint64_t ApicGetTimestampMicros(void)
{
    return (uint64_t)((1000000. * ((double)__atomic_load_n(&ApicCounter, __ATOMIC_SEQ_CST)  + (double)LAPIC(LAPIC_TIMER_INITIAL_COUNT_OFFSET) 
        - (double)LAPIC(LAPIC_TIMER_CURRENT_COUNT_OFFSET))) / (double)ApicFrequency);
}

uint64_t ApicGetTimestampMillis(void)
{
    return (uint64_t)((1000. * ((double)__atomic_load_n(&ApicCounter, __ATOMIC_SEQ_CST) + (double)LAPIC(LAPIC_TIMER_INITIAL_COUNT_OFFSET) 
        - (double)LAPIC(LAPIC_TIMER_CURRENT_COUNT_OFFSET))) / (double)ApicFrequency);
}
#else
uint64_t ApicGetTimestamp(void)
{
    return (uint64_t)((1000000000. * ((double)__atomic_load_n(&ApicCounter[HalGetCurrentCpu()], __ATOMIC_SEQ_CST) 
         + (double)LAPIC(LAPIC_TIMER_INITIAL_COUNT_OFFSET) - (double)LAPIC(LAPIC_TIMER_CURRENT_COUNT_OFFSET))) 
        / (double)ApicFrequency);
}

uint64_t ApicGetTimestampMicros(void)
{
    return (uint64_t)((1000000. * ((double)__atomic_load_n(&ApicCounter[HalGetCurrentCpu()], __ATOMIC_SEQ_CST) 
         + (double)LAPIC(LAPIC_TIMER_INITIAL_COUNT_OFFSET) - (double)LAPIC(LAPIC_TIMER_CURRENT_COUNT_OFFSET))) 
         / (double)ApicFrequency);
}

uint64_t ApicGetTimestampMillis(void)
{
    return (uint64_t)((1000. * ((double)__atomic_load_n(&ApicCounter[HalGetCurrentCpu()], __ATOMIC_SEQ_CST) 
        + (double)LAPIC(LAPIC_TIMER_INITIAL_COUNT_OFFSET) - (double)LAPIC(LAPIC_TIMER_CURRENT_COUNT_OFFSET))) 
        / (double)ApicFrequency);
}
#endif

void ApicSetRealTime(uint64_t realTime)
{
    realTime = (double)realTime * (double)ApicFrequency / 1000.;
#ifndef SMP
    __atomic_store_n(&ApicCounter, realTime, __ATOMIC_SEQ_CST);
#else
    for(uint16_t i = 0; i < MAX_CPU_COUNT; i++)
        __atomic_store_n(&ApicCounter[i], realTime, __ATOMIC_SEQ_CST);
#endif
}

STATUS ApicSetTaskPriority(uint8_t priority)
{
    if(unlikely(NULL == lapic))
        return APIC_LAPIC_NOT_AVAILABLE;
    LAPIC(LAPIC_TPR_OFFSET) = (priority & 0xF) << 4;
    while(((LAPIC(LAPIC_PPR_OFFSET) >> 4) & 0xF) < priority)
        TIGHT_LOOP_HINT();
    return OK;
}

uint8_t ApicGetTaskPriority(void)
{
    if(unlikely(NULL == lapic))
        return 0;
    return (LAPIC(LAPIC_TPR_OFFSET) >> 4) & 0xF;
}

uint8_t ApicGetProcessorPriority(void)
{
    if(unlikely(NULL == lapic))
        return 0;
    return (LAPIC(LAPIC_PPR_OFFSET) >> 4) & 0xF;
}

uint8_t ApicGetCurrentId(void)
{
    return (LAPIC(LAPIC_ID_OFFSET) >> 24);
}

#endif