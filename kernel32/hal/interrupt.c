#include "interrupt.h"
#include "sdrv/pic.h"
#include "sdrv/lapic.h"
#include "sdrv/ioapic.h"
#include "cpu.h"
#include "sdrv/dcpuid.h"
#include "sdrv/pit.h"
#include "it/it.h"
#include "ke/core/panic.h"
#include "common.h"
#include "io/disp/print.h"

static enum HalInterruptMethod interruptMethod = IT_METHOD_NONE;

#define HAL_ISA_INTERRUPT_COUNT 16

static bool dualPicPresent = false;
static uint32_t isaRemapTable[HAL_ISA_INTERRUPT_COUNT];

void HalSetDualPicPresence(bool state)
{
    dualPicPresent = state;
}

STATUS HalAddIsaRemapEntry(uint8_t isaIrq, uint32_t globalIrq)
{
    if(isaIrq >= HAL_ISA_INTERRUPT_COUNT)
        return IT_BAD_VECTOR;
    
    isaRemapTable[isaIrq] = globalIrq;
    return OK;
}

uint32_t HalResolveIsaIrqMapping(uint32_t irq)
{
    if(irq < HAL_ISA_INTERRUPT_COUNT)
        return isaRemapTable[irq];
    
    return irq;
}

STATUS HalInitInterruptController(void)
{
    for(uint8_t i = 0; i < HAL_ISA_INTERRUPT_COUNT; i++)
    {
        isaRemapTable[i] = i;
    }

    PicSetIRQMask(0xFFFF); //mask all PIC interrupts
    PicRemap(HAL_PIC_REMAP_VECTOR, HAL_PIC_REMAP_VECTOR + 8);

    if(CpuidCheckIfApicAvailable()) //APIC available?
    {
        uint32_t lapicAddress = HalGetLapicAddress();
        if(lapicAddress)
        {
            if(OK == ApicInitBSP(lapicAddress)) //try to initialize APIC on bootstrap CPU
            {
                if(OK == ApicIoInit())
                {
                    interruptMethod = IT_METHOD_APIC;
                    return OK;
                }
                else if(dualPicPresent)
                {
                    interruptMethod = IT_METHOD_PIC;
                    return OK;
                }
                else
                    PRINT("No I/O APIC nor dual-PIC is available\n");  
            }
            else
                PRINT("Bootstrap CPU initialization failed\n");
        }
        else
            PRINT("LAPIC address seems to be zero\n");
    }
    IoPrint("Cannot continue: Local APIC is not available on this PC. Boot failed.\n");
    while(1)
    {
        ItDisableInterrupts();
    }
}

STATUS HalRegisterIRQ(uint32_t input, uint8_t vector, enum HalInterruptMode mode,
                        enum HalInterruptPolarity polarity, enum HalInterruptTrigger trigger)
{
    if(IT_METHOD_APIC == interruptMethod)
        return ApicIoRegisterIRQ(input, vector, mode, polarity, trigger);
    else if(IT_METHOD_PIC == interruptMethod)
        return OK;

    return IT_NO_CONTROLLER_CONFIGURED;
}

STATUS HalUnregisterIRQ(uint32_t input)
{
    if(IT_METHOD_APIC == interruptMethod)
        return ApicIoUnregisterIRQ(input);
    else if(IT_METHOD_PIC == interruptMethod)
        return OK;

    return IT_NO_CONTROLLER_CONFIGURED;
}

uint8_t HalGetAssociatedVector(uint32_t input)
{
    if(IT_METHOD_APIC == interruptMethod)
        return ApicIoGetAssociatedVector(input);
    else if(IT_METHOD_PIC == interruptMethod)
        return input + HAL_PIC_REMAP_VECTOR;
    
    return 0;
}

STATUS HalEnableIRQ(uint8_t irq)
{
    if(irq < IT_FIRST_INTERRUPT_VECTOR)
        return IT_BAD_VECTOR;
    
    if(IT_METHOD_APIC == interruptMethod)
        return ApicEnableIRQ(irq);
    else if(IT_METHOD_PIC == interruptMethod)
        return PicEnableIRQ(irq);

    return IT_NO_CONTROLLER_CONFIGURED;
}

STATUS HalDisableIRQ(uint8_t irq)
{
    if(irq < IT_FIRST_INTERRUPT_VECTOR)
        return IT_BAD_VECTOR;
    
    if(IT_METHOD_APIC == interruptMethod)
        return ApicDisableIRQ(irq);
    else if(IT_METHOD_PIC == interruptMethod)
        return PicDisableIRQ(irq);

    return IT_NO_CONTROLLER_CONFIGURED;  
}

STATUS HalClearInterruptFlag(uint8_t irq)
{
    if(irq < IT_FIRST_INTERRUPT_VECTOR)
        return IT_BAD_VECTOR;

    if(IT_METHOD_NONE == interruptMethod)
        return IT_NO_CONTROLLER_CONFIGURED;

    if(IT_METHOD_PIC == interruptMethod)
        PicSendEOI(irq);
    
    return ApicSendEOI();
}

STATUS HalConfigureSystemTimer(uint8_t irq)
{   
    if(IT_METHOD_NONE != interruptMethod)
    {
        return ApicConfigureSystemTimer(irq);
    }

    return IT_NO_CONTROLLER_CONFIGURED;
}

STATUS HalStartSystemTimer(uint64_t time)
{
    if(IT_METHOD_NONE != interruptMethod)
    {
        ApicStartSystemTimer(time);
        return OK;
    }

    return IT_NO_CONTROLLER_CONFIGURED;
}

bool HalIsInterruptSpurious(void)
{
    if(IT_METHOD_APIC == interruptMethod)
        return false;
    else
        return PicIsIrqSpurious();
}

enum HalInterruptMethod HalGetInterruptHandlingMethod(void)
{
    return interruptMethod;
}

STATUS HalSetTaskPriority(uint8_t priority)
{
    return ApicSetTaskPriority(priority);
}

uint8_t HalGetTaskPriority(void)
{
    return ApicGetTaskPriority();
}

uint8_t HalGetProcessorPriority(void)
{
    return ApicGetProcessorPriority();
}