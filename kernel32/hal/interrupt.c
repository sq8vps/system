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

    PicSetIrqMask(0xFFFF); //mask all PIC interrupts
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
    STATUS status = OK;
    uint8_t vector = 0;

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
            && (IT_SHAREABLE == matching->params.shared)
            && (IT_SHAREABLE == params.shared)
            ))
        {
            KeReleaseSpinlock(&HalInterruptListLock);
            return IT_ALREADY_REGISTERED;
        }

        status = ItInstallInterruptHandler(matching->vector, isr, context);
        if(OK == status)
            matching->consumers++;
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

        if(interruptMethod == IT_METHOD_APIC)
        {
            vector = ItReserveVector(IT_VECTOR_ANY);
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
                return status; 
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
        }
        else //8259 PIC
        {
            if(input >= PIC_INPUT_COUNT)
            {
                MmFreeKernelHeap(matching);
                KeReleaseSpinlock(&HalInterruptListLock);
                return IT_VECTOR_NOT_FREE;
            }
            else
            {
                vector = ItReserveVector(input + HAL_PIC_REMAP_VECTOR);
                if(vector != (input + HAL_PIC_REMAP_VECTOR))
                {
                    MmFreeKernelHeap(matching);
                    KeReleaseSpinlock(&HalInterruptListLock);
                    return IT_NO_FREE_VECTORS;
                }
                status = ItInstallInterruptHandler(vector, isr, context);
                if(OK != status)
                {
                    ItFreeVector(vector);
                    MmFreeKernelHeap(matching);
                    KeReleaseSpinlock(&HalInterruptListLock);
                    return status;
                }
            }
        }

    }
    matching->input = input;
    matching->vector = vector;
    matching->params = params;
    matching->consumers = 1;

    if(NULL == HalInterruptList)
        HalInterruptList = matching;
    else
    {
        struct HalInterruptEntry *t = HalInterruptList;
        while(t->next)
            t = t->next;
        t->next = matching;
    }

    KeReleaseSpinlock(&HalInterruptListLock);
    return OK;
}

STATUS HalUnregisterIrq(uint32_t input, ItHandler isr)
{
    STATUS status = OK;

    struct HalInterruptEntry *matching = NULL;
    struct HalInterruptEntry *previous = NULL;
    KeAcquireSpinlock(&HalInterruptListLock);
    if(NULL != HalInterruptList)
    {
        matching = HalInterruptList;
        previous = HalInterruptList;
        while(matching)
        {
            if(matching->input == input)
                break;
            if(NULL != previous->next)
                previous = previous->next;
            matching = matching->next;
        }
    }   
    if(NULL != matching)
    {
        status = ItUninstallInterruptHandler(matching->input, isr);
        if(OK != status)
            goto HalUnregisterIrqExit;

        matching->consumers--;
        if(0 == matching->consumers)
        {
            if(IT_METHOD_APIC == interruptMethod)
            {
                status = ApicIoUnregisterIrq(matching->input);
                if(OK != status)
                    goto HalUnregisterIrqExit;
            }

            if(NULL != previous)
                previous->next = matching->next;
            else
                HalInterruptList = matching->next;
            
            MmFreeKernelHeap(matching);
        }
    }
    else
    {
        status = IT_NOT_REGISTERED;
    }
HalUnregisterIrqExit:
    KeReleaseSpinlock(&HalInterruptListLock);
    return status;
}

uint8_t HalGetAssociatedVector(uint32_t input)
{
    uint8_t vector = 0;
    KeAcquireSpinlock(&HalInterruptListLock);
    struct HalInterruptEntry *t = HalInterruptList;
    while(NULL != t)
    {
        if(t->input == input)
        {
            vector = t->vector;
            break;
        }
        t = t->next;
    }
    KeReleaseSpinlock(&HalInterruptListLock);
    return vector;
}

STATUS HalEnableIrq(uint32_t input, ItHandler isr)
{
    STATUS status = OK;
    KeAcquireSpinlock(&HalInterruptListLock);
    struct HalInterruptEntry *t = HalInterruptList;
    while(NULL != t)
    {
        if(t->input == input)
        {
            break;
        }
        t = t->next;
    }

    if(NULL != t)
    {
        if(IT_METHOD_APIC == interruptMethod)
            status = ApicIoEnableIrq(input);
        else if(IT_METHOD_PIC == interruptMethod)
            status = PicEnableIrq(input);

        if(OK == status)
        {
            status = ItSetInterruptHandlerEnable(t->vector, isr, true);
        }
    }
    else
        status = IT_NOT_REGISTERED;

    KeReleaseSpinlock(&HalInterruptListLock);
    return status;
}

STATUS HalDisableIrq(uint32_t input, ItHandler isr)
{
    STATUS status = OK;
    KeAcquireSpinlock(&HalInterruptListLock);
    struct HalInterruptEntry *t = HalInterruptList;
    while(NULL != t)
    {
        if(t->input == input)
        {
            break;
        }
        t = t->next;
    }

    if(NULL != t)
    {
        if(IT_METHOD_APIC == interruptMethod)
            status = ApicIoDisableIrq(input);
        else if(IT_METHOD_PIC == interruptMethod)
            status = PicDisableIrq(input);

        if(OK == status)
        {
            status = ItSetInterruptHandlerEnable(t->vector, isr, false);
        }
    }
    else
        status = IT_NOT_REGISTERED;

    KeReleaseSpinlock(&HalInterruptListLock);
    return status;
}

uint32_t HalReserveIrq(uint32_t input)
{
    if(IT_METHOD_APIC == interruptMethod)
        return ApicIoReserveInput(input);
    else if(IT_METHOD_PIC == interruptMethod)
        return PicReserveInput(input);
    else
        return IT_NO_CONTROLLER_CONFIGURED;
}

void HalFreeIrq(uint32_t input)
{
    if(IT_METHOD_APIC == interruptMethod)
        ApicIoFreeInput(input);
    else if(IT_METHOD_PIC == interruptMethod)
        PicFreeInput(input);
}

STATUS HalClearInterruptFlag(uint32_t input)
{
    if(IT_METHOD_NONE == interruptMethod)
        return IT_NO_CONTROLLER_CONFIGURED;

    if(IT_METHOD_PIC == interruptMethod)
        PicSendEoi(input);
    
    return ApicSendEoi();
}

STATUS HalConfigureSystemTimer(uint8_t vector)
{   
    if(IT_METHOD_NONE != interruptMethod)
    {
        return ApicConfigureSystemTimer(vector);
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

uint8_t HalRaiseTaskPriority(uint8_t prio)
{
    if(prio > HAL_TASK_PRIORITY_EXCLUSIVE)
        KePanicIPEx(KE_CALLER_ADDRESS(), ILLEGAL_PRIORITY_LEVEL, prio, 0, 0, 0);
    uint8_t old = ApicGetTaskPriority();
    if(prio < old)
        KePanicIPEx(KE_CALLER_ADDRESS(), ILLEGAL_PRIORITY_LEVEL_CHANGE, prio, old, 0, 0);
    ApicSetTaskPriority(prio);
    return old;
}

void HalLowerTaskPriority(uint8_t prio)
{
    if(prio > HAL_TASK_PRIORITY_EXCLUSIVE)
        KePanicIPEx(KE_CALLER_ADDRESS(), ILLEGAL_PRIORITY_LEVEL, prio, 0, 0, 0);
    uint8_t old = ApicGetTaskPriority();
    if(prio > old)
        KePanicIPEx(KE_CALLER_ADDRESS(), ILLEGAL_PRIORITY_LEVEL_CHANGE, prio, old, 0, 0);
    ApicSetTaskPriority(prio);
}

uint8_t HalGetTaskPriority(void)
{
    return ApicGetTaskPriority();
}

uint8_t HalGetProcessorPriority(void)
{
    return ApicGetProcessorPriority();
}