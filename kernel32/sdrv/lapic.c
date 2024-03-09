#include "lapic.h"
#include "dcpuid.h"
#include "hal/cpu.h"
#include "mm/mmio.h"
#include "it/it.h"
#include "ioapic.h"
#include "pit.h"
#include "msr.h"
#include "tsc.h"

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

#define LAPIC_LOCAL_MASK (1 << 16)
#define LAPIC_MODE_NMI (4 << 8)
#define LAPIC_POLARITY_ACTIVE_LOW (1 << 13)
#define LAPIC_POLARITY_ACTIVE_HIGH (0 << 13)
#define LAPIC_TIMER_PERIODIC_FLAG (1 << 17)
#define LAPIC_TIMER_ONE_SHOT_FLAG (0 << 17)
#define LAPIC_TIMER_TSC_DEADLINE_FLAG (2 << 17)

#define LAPIC_VECTOR_MASK 0xFF

//LAPIC configuration space handler
//remains NULL when APIC is unavailable
//use uint8_t for easy well-defined pointer arithmetics
static uint8_t *lapic = NULL;

#define LAPIC_DEFAULT_TIMER_DIVIDER APIC_TIMER_DIVIDE_16
static uint64_t frequency; //calculated clock frequency  

static uint64_t counter = 0;

/**
 * @brief A convenience macro for 32-bit Local APIC register access
*/
#define LAPIC(register) (*(uint32_t volatile*)(lapic + register))

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

STATUS ApicInitAP(void)
{
    if(NULL == lapic)
        return APIC_LAPIC_NOT_AVAILABLE;

    LAPIC(LAPIC_LINT0_OFFSET) = LAPIC_LOCAL_MASK;
    LAPIC(LAPIC_LINT1_OFFSET) = LAPIC_LOCAL_MASK;

    LAPIC(LAPIC_TPR_OFFSET) = 0;
    LAPIC(LAPIC_DESTINATION_FORMAT_OFFSET) = 0xFFFFFFFF;
    LAPIC(LAPIC_LOGICAL_DESTINATION_OFFSET) = 0x01000000;

    LAPIC(LAPIC_SPURIOUS_INTERRUPT_OFFSET) = 0x100 | IT_LAPIC_SPURIOUS_VECTOR;

    return OK;
}

STATUS ApicInitBSP(uintptr_t address)
{
    if(0 == address)
        return APIC_LAPIC_NOT_AVAILABLE;

    lapic = MmMapMmIo(address, MM_PAGE_SIZE);
    if(NULL == lapic)
        return OUT_OF_RESOURCES;

    STATUS ret = OK;
    if(OK != (ret = ApicInitAP()))
    {
        MmUnmapMmIo(lapic);
        lapic = NULL;
        return ret;
    }

    if(OK != (ret = ItInstallInterruptHandler(IT_LAPIC_SPURIOUS_VECTOR, spuriousInterruptHandler, NULL)))
    {
        MmUnmapMmIo(lapic);
        lapic = NULL;
        return ret;
    }

    if(OK != (ret = ItSetInterruptHandlerEnable(IT_LAPIC_SPURIOUS_VECTOR, spuriousInterruptHandler, true)))
    {
        ItUninstallInterruptHandler(IT_LAPIC_SPURIOUS_VECTOR, spuriousInterruptHandler);
        MmUnmapMmIo(lapic);
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

// STATUS ApicEnableIRQ(uint8_t vector)
// {
//     if(vector < IT_FIRST_INTERRUPT_VECTOR)  
//         return IT_BAD_VECTOR;
    
//     if(lapic)
//     {
//         STATUS ret = ApicIoEnableIRQ(vector);
//         if(OK != ret)
//         {
//             if(vector == (LAPIC(LAPIC_LVT_TIMER_OFFSET) & LAPIC_VECTOR_MASK))
//                 LAPIC(LAPIC_LVT_TIMER_OFFSET) &= ~(LAPIC_LOCAL_MASK);
//             else if(vector == (LAPIC(LAPIC_LINT1_OFFSET) & LAPIC_VECTOR_MASK))
//                 LAPIC(LAPIC_LINT1_OFFSET) &= ~(LAPIC_LOCAL_MASK);
//             else if(vector == (LAPIC(LAPIC_LINT0_OFFSET) & LAPIC_VECTOR_MASK))
//                 LAPIC(LAPIC_LINT0_OFFSET) &= ~(LAPIC_LOCAL_MASK);
//             else if(vector == (LAPIC(LAPIC_LVT_CMCI_OFFSET) & LAPIC_VECTOR_MASK))
//                 LAPIC(LAPIC_LVT_CMCI_OFFSET) &= ~(LAPIC_LOCAL_MASK);
//             else if(vector == (LAPIC(LAPIC_LVT_ERROR_OFFSET) & LAPIC_VECTOR_MASK))
//                 LAPIC(LAPIC_LVT_ERROR_OFFSET) &= ~(LAPIC_LOCAL_MASK);
//             else if(vector == (LAPIC(LAPIC_LVT_PERF_MONITORING_OFFSET) & LAPIC_VECTOR_MASK))
//                 LAPIC(LAPIC_LVT_PERF_MONITORING_OFFSET) &= ~(LAPIC_LOCAL_MASK);
//             else if(vector == (LAPIC(LAPIC_LVT_THERMAL_SENSOR_OFFSET) & LAPIC_VECTOR_MASK))
//                 LAPIC(LAPIC_LVT_THERMAL_SENSOR_OFFSET) &= ~(LAPIC_LOCAL_MASK);
//             else
//             {
//                 return IT_NOT_REGISTERED;
//             }

//             return OK;
//         }
//         return OK;
//     }

//     return APIC_LAPIC_NOT_AVAILABLE;
// }

// STATUS ApicDisableIRQ(uint8_t vector)
// {
//     if(vector < IT_FIRST_INTERRUPT_VECTOR)  
//         return IT_BAD_VECTOR;
    
//     if(lapic)
//     {
//         STATUS ret = ApicIoDisableIRQ(vector);
//         if(OK != ret)
//         {
//             if(vector == (LAPIC(LAPIC_LVT_TIMER_OFFSET) & LAPIC_VECTOR_MASK))
//                 LAPIC(LAPIC_LVT_TIMER_OFFSET) |= (LAPIC_LOCAL_MASK);
//             else if(vector == (LAPIC(LAPIC_LINT1_OFFSET) & LAPIC_VECTOR_MASK))
//                 LAPIC(LAPIC_LINT1_OFFSET) |= (LAPIC_LOCAL_MASK);
//             else if(vector == (LAPIC(LAPIC_LINT0_OFFSET) & LAPIC_VECTOR_MASK))
//                 LAPIC(LAPIC_LINT0_OFFSET) |= (LAPIC_LOCAL_MASK);
//             else if(vector == (LAPIC(LAPIC_LVT_CMCI_OFFSET) & LAPIC_VECTOR_MASK))
//                 LAPIC(LAPIC_LVT_CMCI_OFFSET) |= (LAPIC_LOCAL_MASK);
//             else if(vector == (LAPIC(LAPIC_LVT_ERROR_OFFSET) & LAPIC_VECTOR_MASK))
//                 LAPIC(LAPIC_LVT_ERROR_OFFSET) |= (LAPIC_LOCAL_MASK);
//             else if(vector == (LAPIC(LAPIC_LVT_PERF_MONITORING_OFFSET) & LAPIC_VECTOR_MASK))
//                 LAPIC(LAPIC_LVT_PERF_MONITORING_OFFSET) |= (LAPIC_LOCAL_MASK);
//             else if(vector == (LAPIC(LAPIC_LVT_THERMAL_SENSOR_OFFSET) & LAPIC_VECTOR_MASK))
//                 LAPIC(LAPIC_LVT_THERMAL_SENSOR_OFFSET) |= (LAPIC_LOCAL_MASK);
//             else
//             {
//                 return IT_NOT_REGISTERED;
//             }

//             return OK;
//         }
//         return IT_NOT_REGISTERED;
//     }

//     return APIC_LAPIC_NOT_AVAILABLE;
// }

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
        HalMsrSet(MSR_IA32_TSC_DEADLINE, TscCalculateRaw(time * (uint64_t)1000) + TscGetRaw());
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