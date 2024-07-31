#if defined(__i686__) || defined(__amd64__)

#include "lapic.h"
#include "dcpuid.h"
#include "mm/mmio.h"
#include "it/it.h"
#include "ioapic.h"
#include "pit.h"
#include "msr.h"
#include "tsc.h"

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
static uint64_t frequency; //calculated clock frequency  

static uint64_t counter = 0;

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

STATUS ApicSendIpi(uint8_t destination, enum ApicIpiMode mode, uint8_t vector, bool assert)
{
    LAPIC(LAPIC_ICR_OFFSET + 0x10) = (LAPIC(LAPIC_ICR_OFFSET + 0x10) & 0xFF000000) | (destination << 24);
    LAPIC(LAPIC_ICR_OFFSET) = (LAPIC(LAPIC_ICR_OFFSET) & LAPIC_ICR_RESERVED_MASK) 
        | (mode << 8) | (assert ? LAPIC_ICR_LEVEL_ASSERT : LAPIC_ICR_LEVEL_DEASSERT) | vector;
    return OK;
}

void ApicWaitForIpiDelivery(void)
{
    while(LAPIC(LAPIC_ICR_OFFSET) & LAPIC_ICR_DELIVERY_STATUS_BIT)
    {
        TIGHT_LOOP_HINT();
    }
}

STATUS ApicInitAp(void)
{
    if(NULL == lapic)
        return APIC_LAPIC_NOT_AVAILABLE;

    LAPIC(LAPIC_LINT0_OFFSET) = LAPIC_LOCAL_MASK;
    LAPIC(LAPIC_LINT1_OFFSET) = LAPIC_LOCAL_MASK;

    LAPIC(LAPIC_TPR_OFFSET) = 0;
    LAPIC(LAPIC_DESTINATION_FORMAT_OFFSET) = 0xFFFFFFFF;
    LAPIC(LAPIC_LOGICAL_DESTINATION_OFFSET) = 0x01000000;

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
    frequency = ((uint64_t)100) * ((uint64_t)(0xFFFFFFFF - value));
    
    return OK;
}

STATUS ApicInit(uintptr_t address)
{
    if(0 == address)
        return APIC_LAPIC_NOT_AVAILABLE;

    lapic = MmMapMmIo(address, MM_PAGE_SIZE);
    if(NULL == lapic)
        return OUT_OF_RESOURCES;
    return OK;
}

STATUS ApicConfigureSystemTimer(uint8_t vector)
{
    if(vector < IT_FIRST_INTERRUPT_VECTOR)  
        return IT_BAD_VECTOR;
    
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
        counter += ((uint64_t)LAPIC(LAPIC_TIMER_INITIAL_COUNT_OFFSET) - (uint64_t)LAPIC(LAPIC_TIMER_CURRENT_COUNT_OFFSET));
        LAPIC(LAPIC_TIMER_INITIAL_COUNT_OFFSET) = time * frequency / (uint64_t)1000000;
    }
    LAPIC(LAPIC_LVT_TIMER_OFFSET) &= ~LAPIC_LOCAL_MASK;
}

uint64_t ApicGetTimestamp(void)
{
    return (uint64_t)((1000000000. * ((double)counter + (double)LAPIC(LAPIC_TIMER_INITIAL_COUNT_OFFSET) 
        - (double)LAPIC(LAPIC_TIMER_CURRENT_COUNT_OFFSET))) / (double)frequency);
}

uint64_t ApicGetTimestampMicros(void)
{
    return (uint64_t)((1000000. * ((double)counter + (double)LAPIC(LAPIC_TIMER_INITIAL_COUNT_OFFSET) 
        - (double)LAPIC(LAPIC_TIMER_CURRENT_COUNT_OFFSET))) / (double)frequency);
}

uint64_t ApicGetTimestampMillis(void)
{
    return (uint64_t)((1000. * ((double)counter + (double)LAPIC(LAPIC_TIMER_INITIAL_COUNT_OFFSET) 
        - (double)LAPIC(LAPIC_TIMER_CURRENT_COUNT_OFFSET))) / (double)frequency);
}

STATUS ApicSetTaskPriority(uint8_t priority)
{
    if(NULL == lapic)
        return APIC_LAPIC_NOT_AVAILABLE;
    
    LAPIC(LAPIC_TPR_OFFSET) = (priority & 0xF) << 4;
    return OK;
}

uint8_t ApicGetTaskPriority(void)
{
    if(NULL == lapic)
        return 0;
    
    return (LAPIC(LAPIC_TPR_OFFSET) >> 4) & 0xFF;
}

uint8_t ApicGetProcessorPriority(void)
{
    if(NULL == lapic)
        return 0;
    
    return (LAPIC(LAPIC_PPR_OFFSET) >> 4) & 0xFF;
}

#endif