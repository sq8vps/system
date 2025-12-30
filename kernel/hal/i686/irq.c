
#include "hal/interrupt.h"
#include "pic.h"
#include "ioapic.h"
#include "lapic.h"
#include "ke/core/panic.h"
#include "irq.h"

#define ISA_INTERRUPT_COUNT 16
#define PIC_REMAP_VECTOR IT_IRQ_VECTOR_BASE

static bool DualPicPresent = false;
static bool UseIoApic = false;

static struct
{
    uint32_t gsi;
    struct HalInterruptParams params;
}
IsaRemapTable[ISA_INTERRUPT_COUNT];

STATUS I686InitInterruptController(void)
{
    PicDisable();
    PicRemap(PIC_REMAP_VECTOR, PIC_REMAP_VECTOR + 8);

    if(OK == ApicInitBsp()) //try to initialize APIC on bootstrap CPU
    {
        if(OK == ApicIoInit())
        {
            UseIoApic = true;
            return OK;
        }
        else if(DualPicPresent)
        {
            PicEnable();
            UseIoApic = false;
            return OK;
        }
        else
            FAIL_BOOT("no I/O APIC nor dual PIC available");
    }
    else
        FAIL_BOOT("bootstrap CPU local APIC initialization failed");
    
    return OK;
}

void I686SetDualPicPresence(bool state)
{
    DualPicPresent = state;
}

void I686SetDefaultIsaRemap(void)
{
    for(uint16_t i = 0; i < ISA_INTERRUPT_COUNT; i++)
        I686AddIsaRemapEntry(i, i, (struct HalInterruptParams){
            .mode = HAL_IT_MODE_FIXED,
            .polarity = HAL_IT_POLARITY_ACTIVE_HIGH,
            .trigger = HAL_IT_TRIGGER_EDGE,
            .shared = HAL_IT_NOT_SHAREABLE,
            .wake = HAL_IT_WAKE_INCAPABLE
        });
}

STATUS I686AddIsaRemapEntry(uint8_t isaIrq, uint32_t gsi, struct HalInterruptParams params)
{
    if(isaIrq >= ISA_INTERRUPT_COUNT)
        return BAD_PARAMETER;
    
    IsaRemapTable[isaIrq].gsi = gsi;
    IsaRemapTable[isaIrq].params = params;
    return OK;
}

uint32_t I686ResolveIsaIrqMapping(uint32_t irq)
{
    if(UseIoApic)
    {
        if(irq < ISA_INTERRUPT_COUNT)
            return IsaRemapTable[irq].gsi;
    }
    
    return irq;
}

struct HalInterruptParams I686ResolveIsaIrqParams(uint32_t irq)
{
    if(UseIoApic)
    {
        if(irq < ISA_INTERRUPT_COUNT)
            return IsaRemapTable[irq].params;
    }
    
    return (struct HalInterruptParams){
        .mode = HAL_IT_MODE_FIXED,
        .polarity = HAL_IT_POLARITY_ACTIVE_HIGH,
        .trigger = HAL_IT_TRIGGER_EDGE,
        .shared = HAL_IT_NOT_SHAREABLE,
        .wake = HAL_IT_WAKE_INCAPABLE
    };
}

bool I686IsIoApicUsed(void)
{
    return UseIoApic;
}

STATUS HalArchEnableIrq(uint32_t input)
{
    if(UseIoApic)
        return ApicIoEnableIrq(input);
    else
        return PicEnableIrq(input);
}

STATUS HalArchDisableIrq(uint32_t input)
{
    if(UseIoApic)
        return ApicIoDisableIrq(input);
    else
        return PicDisableIrq(input);
}

uint32_t HalReserveIrq(uint32_t input)
{
    if(UseIoApic)
        return ApicIoReserveInput(input);
    else
        return PicReserveInput(input);
}

void HalFreeIrq(uint32_t input)
{
    if(UseIoApic)
        ApicIoFreeInput(input);
    else
        PicFreeInput(input);
}

STATUS HalClearInterruptFlag(uint32_t vector)
{
    if(!UseIoApic)
        PicSendEoi(vector - PIC_REMAP_VECTOR);
    
    return ApicSendEoi();
}

bool HalIsInterruptSpurious(uint8_t vector)
{
    if(UseIoApic)
        return false;
    else
        return PicIsIrqSpurious(vector);
}

STATUS HalArchRegiserIrq(uint32_t input, uint8_t vector, struct HalInterruptParams params)
{
    if(UseIoApic)
    {
        return ApicIoRegisterIrq(input, vector, params.mode, params.polarity, params.trigger);
    }
    else //8259 PIC
    {
        if(input >= PIC_INPUT_COUNT)
        {
            return BAD_PARAMETER;
        }
        else
            return OK;
    }    
}

STATUS HalArchUnregisterIrq(uint32_t input)
{
    if(UseIoApic)
        return ApicIoUnregisterIrq(input);
    else
        return OK;
}

uint32_t HalIrqVectorFromIrq(uint32_t irq)
{
    if(UseIoApic)
        return irq;
    else
        return irq + PIC_REMAP_VECTOR;
}

bool HalIrqIsVectorRelatedToIrq(void)
{
    return !UseIoApic;
}

void HalSetTaskPriority(PRIO prio)
{
    ApicSetTaskPriority(prio);
}

PRIO HalGetTaskPriority(void)
{
    return ApicGetTaskPriority();
}

PRIO HalGetProcessorPriority(void)
{
    return ApicGetProcessorPriority();
}
