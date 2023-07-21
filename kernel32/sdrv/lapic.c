#include "lapic.h"
#include "cpuid.h"
#include "hal/cpu.h"
#include "mm/mmio.h"
#include "it/it.h"
#include "ioapic.h"
#include "pit.h"

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

#define LAPIC_SPURIOUS_INTERRUPT_IRQ 255

#define LAPIC_LOCAL_MASK (1 << 16)
#define LAPIC_MODE_NMI (4 << 8)
#define LAPIC_POLARITY_ACTIVE_LOW (1 << 13)
#define LAPIC_POLARITY_ACTIVE_HIGH (0 << 13)
#define LAPIC_TIMER_PERIODIC_FLAG (1 << 17)
#define LAPIC_TIMER_ONE_SHOT_FLAG (0 << 17)

#define LAPIC_VECTOR_MASK 0xFF

//LAPIC configuration space handler
//remains NULL when APIC is unavailable
//use uint8_t for easy well-defined pointer arithmetics
static uint8_t *lapic = NULL;

#define LAPIC_DEFAULT_TIMER_DIVIDER APIC_TIMER_DIVIDE_16
static uint64_t busFrequency; //calculated bus frequency  

#define LAPIC(register) (*(uint32_t volatile*)(lapic + register))

IT_HANDLER static void spuriousInterruptHandler(struct ItFrame *f)
{

}

STATUS ApicSendEOI(void)
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

void ApicUpdateTaskPriority(uint8_t priority)
{
    if(NULL != lapic)
        LAPIC(LAPIC_TPR_OFFSET) = priority & 0xF;
}

STATUS ApicInitAP(void)
{
    if(NULL == lapic)
        return APIC_LAPIC_NOT_AVAILABLE;

    LAPIC(LAPIC_LINT0_OFFSET) = LAPIC_LOCAL_MASK;
    LAPIC(LAPIC_LINT1_OFFSET) = LAPIC_LOCAL_MASK;

    LAPIC(LAPIC_TPR_OFFSET) = 0;
    LAPIC(LAPIC_DESTINATION_FORMAT_OFFSET) = 0xFFFFFFFF;
    LAPIC(LAPIC_LOGICAL_DESTINATION_OFFSET) = 0x01000000;

    LAPIC(LAPIC_SPURIOUS_INTERRUPT_OFFSET) = 0x100 | LAPIC_SPURIOUS_INTERRUPT_IRQ;

    return OK;
}

STATUS ApicInitBSP(uintptr_t address)
{
    if(0 == address)
        return APIC_LAPIC_NOT_AVAILABLE;

    lapic = MmMapMmIo(address, MM_PAGE_SIZE);
    if(NULL == lapic)
        return MM_DYNAMIC_MEMORY_ALLOCATION_FAILURE;

    STATUS ret = OK;
    if(OK != (ret = ApicInitAP()))
    {
        MmUnmapMmIo(lapic, MM_PAGE_SIZE);
        return ret;
    }

    if(OK != (ret = ItInstallInterruptHandler(LAPIC_SPURIOUS_INTERRUPT_IRQ, spuriousInterruptHandler, PL_KERNEL)))
    {
        MmUnmapMmIo(lapic, MM_PAGE_SIZE);
        return ret;
    }

    LAPIC(LAPIC_TIMER_DIVIDER_OFFSET) = LAPIC_DEFAULT_TIMER_DIVIDER & 0b1011;
    PitOneShotInit(10000); //10000us=10ms
    busFrequency = (uint64_t)16 * (uint64_t)100 *
        (uint64_t)PitOneShotMeasure(
            (uint32_t*)(lapic + LAPIC_TIMER_CURRENT_COUNT_OFFSET), 
            (uint32_t*)(lapic + LAPIC_TIMER_INITIAL_COUNT_OFFSET), 
            0xFFFFFFFF);
    
    return OK;
}

STATUS ApicEnableIRQ(uint8_t vector)
{
    if(vector < IT_FIRST_INTERRUPT_VECTOR)  
        return IT_BAD_VECTOR;
    
    if(lapic)
    {
        STATUS ret = ApicIoEnableIRQ(vector);
        if(OK != ret)
        {
            if(vector == (LAPIC(LAPIC_LVT_TIMER_OFFSET) & LAPIC_VECTOR_MASK))
                LAPIC(LAPIC_LVT_TIMER_OFFSET) &= ~(LAPIC_LOCAL_MASK);
            else if(vector == (LAPIC(LAPIC_LINT1_OFFSET) & LAPIC_VECTOR_MASK))
                LAPIC(LAPIC_LINT1_OFFSET) &= ~(LAPIC_LOCAL_MASK);
            else if(vector == (LAPIC(LAPIC_LINT0_OFFSET) & LAPIC_VECTOR_MASK))
                LAPIC(LAPIC_LINT0_OFFSET) &= ~(LAPIC_LOCAL_MASK);
            else if(vector == (LAPIC(LAPIC_LVT_CMCI_OFFSET) & LAPIC_VECTOR_MASK))
                LAPIC(LAPIC_LVT_CMCI_OFFSET) &= ~(LAPIC_LOCAL_MASK);
            else if(vector == (LAPIC(LAPIC_LVT_ERROR_OFFSET) & LAPIC_VECTOR_MASK))
                LAPIC(LAPIC_LVT_ERROR_OFFSET) &= ~(LAPIC_LOCAL_MASK);
            else if(vector == (LAPIC(LAPIC_LVT_PERF_MONITORING_OFFSET) & LAPIC_VECTOR_MASK))
                LAPIC(LAPIC_LVT_PERF_MONITORING_OFFSET) &= ~(LAPIC_LOCAL_MASK);
            else if(vector == (LAPIC(LAPIC_LVT_THERMAL_SENSOR_OFFSET) & LAPIC_VECTOR_MASK))
                LAPIC(LAPIC_LVT_THERMAL_SENSOR_OFFSET) &= ~(LAPIC_LOCAL_MASK);
            else
            {
                RETURN(IT_NOT_REGISTERED);
            }

            return OK;
        }
        RETURN(IT_NOT_REGISTERED);
    }

    return APIC_LAPIC_NOT_AVAILABLE;
}

STATUS ApicDisableIRQ(uint8_t vector)
{
    if(vector < IT_FIRST_INTERRUPT_VECTOR)  
        return IT_BAD_VECTOR;
    
    if(lapic)
    {
        STATUS ret = ApicIoDisableIRQ(vector);
        if(OK != ret)
        {
            if(vector == (LAPIC(LAPIC_LVT_TIMER_OFFSET) & LAPIC_VECTOR_MASK))
                LAPIC(LAPIC_LVT_TIMER_OFFSET) |= (LAPIC_LOCAL_MASK);
            else if(vector == (LAPIC(LAPIC_LINT1_OFFSET) & LAPIC_VECTOR_MASK))
                LAPIC(LAPIC_LINT1_OFFSET) |= (LAPIC_LOCAL_MASK);
            else if(vector == (LAPIC(LAPIC_LINT0_OFFSET) & LAPIC_VECTOR_MASK))
                LAPIC(LAPIC_LINT0_OFFSET) |= (LAPIC_LOCAL_MASK);
            else if(vector == (LAPIC(LAPIC_LVT_CMCI_OFFSET) & LAPIC_VECTOR_MASK))
                LAPIC(LAPIC_LVT_CMCI_OFFSET) |= (LAPIC_LOCAL_MASK);
            else if(vector == (LAPIC(LAPIC_LVT_ERROR_OFFSET) & LAPIC_VECTOR_MASK))
                LAPIC(LAPIC_LVT_ERROR_OFFSET) |= (LAPIC_LOCAL_MASK);
            else if(vector == (LAPIC(LAPIC_LVT_PERF_MONITORING_OFFSET) & LAPIC_VECTOR_MASK))
                LAPIC(LAPIC_LVT_PERF_MONITORING_OFFSET) |= (LAPIC_LOCAL_MASK);
            else if(vector == (LAPIC(LAPIC_LVT_THERMAL_SENSOR_OFFSET) & LAPIC_VECTOR_MASK))
                LAPIC(LAPIC_LVT_THERMAL_SENSOR_OFFSET) |= (LAPIC_LOCAL_MASK);
            else
            {
                RETURN(IT_NOT_REGISTERED);
            }

            return OK;
        }
        RETURN(IT_NOT_REGISTERED);
    }

    return APIC_LAPIC_NOT_AVAILABLE;
}

STATUS ApicInitSystemTimer(uint32_t interval, uint8_t vector)
{
    if(vector < IT_FIRST_INTERRUPT_VECTOR)  
        return IT_BAD_VECTOR;
    
    if(lapic)
    {
        LAPIC(LAPIC_TIMER_DIVIDER_OFFSET) = APIC_TIMER_DIVIDE_16 & 0x0b1011;
        LAPIC(LAPIC_LVT_TIMER_OFFSET) = LAPIC_TIMER_PERIODIC_FLAG | LAPIC_LOCAL_MASK | vector;
        LAPIC(LAPIC_TIMER_INITIAL_COUNT_OFFSET) = (((uint64_t)interval) * ((uint64_t)busFrequency)) / ((uint64_t)16 * (uint64_t)1000);
        return OK;
    }

    return APIC_LAPIC_NOT_AVAILABLE;   
}

void ApicRestartSystemTimer(void)
{
    uint32_t val = LAPIC(LAPIC_TIMER_INITIAL_COUNT_OFFSET);
    LAPIC(LAPIC_TIMER_INITIAL_COUNT_OFFSET) = val;
}