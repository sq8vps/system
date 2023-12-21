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
#include "ke/core/mutex.h"
#include "mm/heap.h"

static enum HalInterruptMethod interruptMethod = IT_METHOD_NONE;

#define HAL_ISA_INTERRUPT_COUNT 16

static bool dualPicPresent = false;
static uint32_t isaRemapTable[HAL_ISA_INTERRUPT_COUNT];

struct HalInterruptEntry
{
    uint32_t input;
    uint8_t vector;
    struct HalInterruptParams params;
    uint8_t consumers;

    struct HalInterruptEntry *next;
} static *HalInterruptList = NULL;
static KeSpinlock HalInterruptListLock = KeSpinlockInitializer;

void HalSetDualPicPresence(bool state)
{
    dualPicPresent = state;
}

STATUS HalAddIsaRemapEntry(uint8_t isaIrq, uint32_t gsi)
{
    if(isaIrq >= HAL_ISA_INTERRUPT_COUNT)
        return IT_BAD_VECTOR;
    
    isaRemapTable[isaIrq] = gsi;
    return OK;
}

uint32_t HalResolveIsaIrqMapping(uint32_t irq)
{
    if(IT_METHOD_APIC == interruptMethod)
    {
        if(irq < HAL_ISA_INTERRUPT_COUNT)
            return isaRemapTable[irq];
    }
    
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

STATUS HalRegisterIrq(
    uint32_t input,
    ItHandler isr,
    void *context,
    struct HalInterruptParams params)
{
    STATUS status;

    struct HalInterruptEntry *matching = NULL;
    KeAcquireSpinlock(&HalInterruptListLock);
    if(NULL != HalInterruptList)
    {
        matching = HalInterruptList;
        while(matching)
        {
            if(matching->input == input)
                break;
            matching = matching->next;
        }
    }

    if(NULL != matching)
    {
        if(!(
            (matching->params.mode == params.mode)
            && (matching->params.polarity == params.polarity)
            && (matching->params.trigger == params.trigger)
            && (IT_SHARED == matching->params.shared)
            && (IT_SHARED == params.shared)
            ))
        {
            KeReleaseSpinlock(&HalInterruptListLock);
            return IT_ALREADY_REGISTERED;
        }
    }
    else
    {
        matching = MmAllocateKernelHeap(sizeof(*matching));
        if(NULL == matching)
        {
            KeReleaseSpinlock(&HalInterruptListLock);
            return OUT_OF_RESOURCES;
        }
        CmMemset(matching, 0, sizeof(*matching));
        // if(NULL == HalInterruptList)
        //     HalInterruptList = matching;
        // else
        // {
        //     struct HalInterruptEntry *t = HalInterruptList;
        //     while(t->next)
        //         t = t->next;
        //     t->next = matching;
        // }

        if(interruptMethod == IT_METHOD_APIC)
        {
            uint8_t vector = ItReserveVector(IT_VECTOR_ANY);
            if(0 == vector)
            {
                MmFreeKernelHeap(matching);
                KeReleaseSpinlock(&HalInterruptListLock);
                return OUT_OF_RESOURCES;
            }
            status = ApicIoRegisterIrq(input, vector, params.mode, params.polarity, params.trigger);
            if(OK != status)
            {
                ItFreeVector(vector);
                MmFreeKernelHeap(matching);
                KeReleaseSpinlock(&HalInterruptListLock);
                return OUT_OF_RESOURCES; 
            }
            status = ItInstallInterruptHandler(vector, isr, context);
            if(OK != status)
            {
                ApicIoUnregisterIrq(input);
                ItFreeVector(vector);
                MmFreeKernelHeap(matching);
                KeReleaseSpinlock(&HalInterruptListLock);
                return status;
            }
            matching->input = input;
            matching->vector = vector;
            matching->params = params;
            matching->consumers = 1;
        }
        else
        {
            
        }

    }


    KeReleaseSpinlock(&HalInterruptListLock);
    return status;
}

STATUS HalUnregisterIrq(
uint32_t input
)
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