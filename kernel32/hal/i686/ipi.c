#include "ipi.h"
#include "lapic.h"
#include "it/it.h"
#include "config.h"
#include "ke/core/mutex.h"
#include "mm/slab.h"
#include "hal/arch.h"
#include "ke/core/panic.h"
#include "interrupts/exceptions.h"
#include "memory.h"
#include "ke/sched/sched.h"

#define I686_IPI_SLOT_COUNT 16
#define I686_IPI_INTERRUPT_VECTOR 224

struct
{
    struct I686IpiData data[I686_IPI_SLOT_COUNT];
    volatile uint32_t slotsReserved;
    volatile uint32_t slotsFilled;
    volatile uint16_t remainingAcks;
} static I686IpiState[MAX_CPU_COUNT] = {[0 ... MAX_CPU_COUNT - 1] = {.slotsReserved = 0, .slotsFilled = 0}};

static bool I686IpiInitialized = false;

static STATUS I686HandleIpi(void *context)
{
    uint16_t cpu = HalGetCurrentCpu();

    uint32_t slots;
    while(0 != (slots = __atomic_load_n(&(I686IpiState[cpu].slotsFilled), __ATOMIC_SEQ_CST)))
    {
        for(uint8_t i = 0; i < I686_IPI_SLOT_COUNT; i++)
        {
            if(slots & (1 << i))
            {
                switch(I686IpiState[cpu].data[i].type)
                {
                    case I686_IPI_TLB_SHOOTDOWN:
                        if(!I686IpiState[cpu].data[i].payload.tlb.kernel)
                        {
                            if(KeGetCurrentTask()->cpu.cr3 != I686IpiState[cpu].data[i].payload.tlb.cr3)
                                break;
                        }
                        while(I686IpiState[cpu].data[i].payload.tlb.count)
                        {
                            I686_INVALIDATE_TLB(I686IpiState[cpu].data[i].payload.tlb.address);
                            I686IpiState[cpu].data[i].payload.tlb.address += MM_PAGE_SIZE;
                            --I686IpiState[cpu].data[i].payload.tlb.count;
                        }
                        break;
                    case I686_IPI_CPU_SHUTDOWN:
                        __atomic_fetch_sub(I686IpiState[cpu].data[i].remainingAcks, 1, __ATOMIC_SEQ_CST);
                        ASM("cli");
                        ASM("hlt");
                        while(1)
                            ;
                        break;

                    default:
                        KePanicEx(KERNEL_MODE_FAULT, IPI_UNKNOWN_TYPE, I686IpiState[cpu].data[i].source, 
                            cpu, I686IpiState[cpu].data[i].type);
                        break;
                }
                __atomic_fetch_sub(I686IpiState[cpu].data[i].remainingAcks, 1, __ATOMIC_SEQ_CST);
                __atomic_fetch_and(&(I686IpiState[cpu].slotsFilled), ~((uint32_t)1 << i), __ATOMIC_SEQ_CST);
                __atomic_fetch_and(&(I686IpiState[cpu].slotsReserved), ~((uint32_t)1 << i), __ATOMIC_SEQ_CST);
            }
        }
    }

    return OK;
}

STATUS I686InitializeIpi(void)
{
    STATUS status = OK;

    status = ItInstallInterruptHandler(I686_IPI_INTERRUPT_VECTOR, I686HandleIpi, NULL);
    if(OK == status)
        status = ItSetInterruptHandlerEnable(I686_IPI_INTERRUPT_VECTOR, I686HandleIpi, true);
    
    if(OK == status)
        I686IpiInitialized = true;
    return status;
}

static inline uint16_t I686ReserveIpiSlot(uint16_t cpu)
{
    uint16_t slot = 0;
    uint16_t mask = 1;
    //loop until a free slot is found, that is, the previous
    //bit state was 0 and we set the bit to 1
    while(__atomic_fetch_or(&(I686IpiState[cpu].slotsReserved), mask, __ATOMIC_SEQ_CST) & mask)
    {
        if(slot == (I686_IPI_SLOT_COUNT - 1))
        {
            mask = 1;
            slot = 0;
        }
        else
        {
            mask <<= 1;
            slot++;
        }
    }
    return slot;
}

void I686SendInvalidateTlb(const HalCpuBitmap *targets, uintptr_t cr3, uintptr_t address, uintptr_t pages)
{
    if(!I686IpiInitialized)
        return;
    uint16_t cpu = HalGetCurrentCpu();

    PRIO prio = HalRaisePriorityLevel(HAL_PRIORITY_LEVEL_HIGHEST);

    HAL_GET_CPU_BIT_COUNT(targets, I686IpiState[cpu].remainingAcks);

    if(HalGetCpuCount() < I686IpiState[cpu].remainingAcks)
        I686IpiState[cpu].remainingAcks = HalGetCpuCount();

    --I686IpiState[cpu].remainingAcks;

    if(unlikely(0 == I686IpiState[cpu].remainingAcks))
    {
        HalLowerPriorityLevel(prio);
        return;
    }

    for(uint16_t i = 0; i < HalGetCpuCount(); i++)
    {
        if(i == cpu)
            continue;
        if(HAL_GET_CPU_BIT(targets, i))
        {
            uint16_t slot = I686ReserveIpiSlot(i);
            I686IpiState[i].data[slot].type = I686_IPI_TLB_SHOOTDOWN;
            I686IpiState[i].data[slot].source = cpu;
            I686IpiState[i].data[slot].payload.tlb.address = address;
            I686IpiState[i].data[slot].payload.tlb.count = pages;
            I686IpiState[i].data[slot].payload.tlb.cr3 = cr3;
            I686IpiState[i].data[slot].remainingAcks = &(I686IpiState[cpu].remainingAcks);
            I686IpiState[i].data[slot].payload.tlb.kernel = false;

            __atomic_fetch_or(&(I686IpiState[i].slotsFilled), 1 << slot, __ATOMIC_SEQ_CST);

            ApicSendIpi(APIC_IPI_DESTINATION_NORMAL, HalGetCpuEntry(i)->extensions.lapicId, 
                APIC_IPI_FIXED, I686_IPI_INTERRUPT_VECTOR, true);
            if(OK != ApicWaitForIpiDelivery(US_TO_NS(100)))
                KePanicEx(KERNEL_MODE_FAULT, IPI_DELIVERY_TIMEOUT, cpu, i, I686_IPI_TLB_SHOOTDOWN);

        }
    }

    HalLowerPriorityLevel(prio);

    while(0 != __atomic_load_n(&(I686IpiState[cpu].remainingAcks), __ATOMIC_SEQ_CST))
        TIGHT_LOOP_HINT();
}

void I686SendInvalidateKernelTlb(uintptr_t address, uintptr_t pages)
{
    if(!I686IpiInitialized)
        return;
    uint16_t cpu = HalGetCurrentCpu();

    I686IpiState[cpu].remainingAcks = HalGetCpuCount() - 1;

    PRIO prio = HalRaisePriorityLevel(HAL_PRIORITY_LEVEL_HIGHEST);
    for(uint16_t i = 0; i < HalGetCpuCount(); i++)
    {
        if(i == cpu)
            continue;
        uint16_t slot = I686ReserveIpiSlot(i);

        I686IpiState[i].data[slot].type = I686_IPI_TLB_SHOOTDOWN;
        I686IpiState[i].data[slot].source = cpu;
        I686IpiState[i].data[slot].payload.tlb.address = address;
        I686IpiState[i].data[slot].payload.tlb.count = pages;
        I686IpiState[i].data[slot].remainingAcks = &(I686IpiState[cpu].remainingAcks);
        I686IpiState[i].data[slot].payload.tlb.kernel = true;

        __atomic_fetch_or(&(I686IpiState[i].slotsFilled), 1 << slot, __ATOMIC_SEQ_CST);
    }

    ApicSendIpi(APIC_IPI_DESTINATION_ALL_BUT_SELF, 0, 
        APIC_IPI_FIXED, I686_IPI_INTERRUPT_VECTOR, true);
    if(OK != ApicWaitForIpiDelivery(US_TO_NS(500)))
        KePanicEx(KERNEL_MODE_FAULT, IPI_DELIVERY_TIMEOUT, cpu, UINTPTR_MAX, I686_IPI_TLB_SHOOTDOWN);

    HalLowerPriorityLevel(prio);

    while(0 != __atomic_load_n(&(I686IpiState[cpu].remainingAcks), __ATOMIC_SEQ_CST))
        TIGHT_LOOP_HINT();
}

void I686SendShutdownCpus(void)
{
    if(!I686IpiInitialized)
        return;
    uint16_t cpu = HalGetCurrentCpu();

    I686IpiState[cpu].remainingAcks = HalGetCpuCount() - 1;

    PRIO prio = HalRaisePriorityLevel(HAL_PRIORITY_LEVEL_HIGHEST);
    for(uint16_t i = 0; i < HalGetCpuCount(); i++)
    {
        if(i == cpu)
            continue;
        uint16_t slot = I686ReserveIpiSlot(i);

        I686IpiState[i].data[slot].type = I686_IPI_CPU_SHUTDOWN;
        I686IpiState[i].data[slot].source = cpu;
        I686IpiState[i].data[slot].remainingAcks = &(I686IpiState[cpu].remainingAcks);
        
        __atomic_fetch_or(&(I686IpiState[i].slotsFilled), 1 << slot, __ATOMIC_SEQ_CST);
    }

    ApicSendIpi(APIC_IPI_DESTINATION_ALL_BUT_SELF, 0, 
        APIC_IPI_FIXED, I686_IPI_INTERRUPT_VECTOR, true);
    if(OK != ApicWaitForIpiDelivery(US_TO_NS(500)))
        KePanicEx(KERNEL_MODE_FAULT, IPI_DELIVERY_TIMEOUT, cpu, UINTPTR_MAX, I686_IPI_CPU_SHUTDOWN);

    HalLowerPriorityLevel(prio);

    while(0 != __atomic_load_n(&(I686IpiState[cpu].remainingAcks), __ATOMIC_SEQ_CST))
        TIGHT_LOOP_HINT();
}