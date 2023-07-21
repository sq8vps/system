#include "interrupt.h"
#include "sdrv/pic.h"
#include "sdrv/lapic.h"
#include "sdrv/ioapic.h"
#include "cpu.h"
#include "sdrv/cpuid.h"
#include "sdrv/pit.h"
#include "it/it.h"
#include "ke/panic.h"

static enum HalInterruptMethod interruptMethod = IT_METHOD_NONE;

STATUS HalInitInterruptController(void)
{
    PicSetIRQMask(0xFFFF); //mask all PIC interrupts
    PicRemap(IT_FIRST_INTERRUPT_VECTOR, IT_FIRST_INTERRUPT_VECTOR + 8); //remap PIC interrupts to start at 0x20 (32)

    if(CpuidCheckIfApicAvailable()) //APIC available?
    {
        uint32_t lapicAddress = HalGetLapicAddress();
        if(lapicAddress)
        {
            if(OK == ApicInitBSP(lapicAddress)) //try to initialize APIC on bootstrap CPU
            {
                if(OK == ApicIoInit())
                {
                    PRINT("APIC controller used\n");
                    interruptMethod = IT_METHOD_APIC; //APIC is used on successful initialization
                }
            }
        }
    }
    else //APIC unavailable, fallback to PIC
    {
        PRINT("Legacy PIC controller used\n");
        interruptMethod = IT_METHOD_PIC;
    }

    return OK;
}

STATUS HalEnableIRQ(uint8_t irq)
{
    if(irq < IT_FIRST_INTERRUPT_VECTOR)
        return IT_BAD_VECTOR;
    
    if(IT_METHOD_APIC == interruptMethod)
        return ApicEnableIRQ(irq);
    else if(IT_METHOD_PIC == interruptMethod)
        return PicEnableIRQ(irq - IT_FIRST_INTERRUPT_VECTOR);

    return IT_NO_CONTROLLER_CONFIGURED;
}

STATUS HalDisableIRQ(uint8_t irq)
{
    if(irq < IT_FIRST_INTERRUPT_VECTOR)
        return IT_BAD_VECTOR;
    
    if(IT_METHOD_APIC == interruptMethod)
        return ApicDisableIRQ(irq);
    else if(IT_METHOD_PIC == interruptMethod)
        return PicDisableIRQ(irq - IT_FIRST_INTERRUPT_VECTOR);

    return IT_NO_CONTROLLER_CONFIGURED;  
}

STATUS HalClearInterruptFlag(uint8_t irq)
{
    if(irq < IT_FIRST_INTERRUPT_VECTOR)
        return IT_BAD_VECTOR;

    if(IT_METHOD_APIC == interruptMethod)
        return ApicSendEOI();
    else if(IT_METHOD_PIC == interruptMethod)
        return PicSendEOI(irq - IT_FIRST_INTERRUPT_VECTOR);

    return IT_NO_CONTROLLER_CONFIGURED;
}

STATUS HalInitSystemTimer(uint32_t interval, uint8_t irq)
{   
    if(IT_METHOD_APIC == interruptMethod)
    {
        return ApicInitSystemTimer(interval, irq);
    }
    else if(IT_METHOD_PIC == interruptMethod)
    {
        PitSetInterval(interval);
        return OK;
    }

    return IT_NO_CONTROLLER_CONFIGURED;
}

STATUS HalRestartSystemTimer()
{
    if(IT_METHOD_APIC == interruptMethod)
    {
        ApicRestartSystemTimer();
        return OK;
    }
    else if(IT_METHOD_PIC == interruptMethod)
    {
        return OK; //no way to restart PIT
    }

    return IT_NO_CONTROLLER_CONFIGURED;
}

enum HalInterruptMethod HalGetInterruptHandlingMethod(void)
{
    return interruptMethod;
}