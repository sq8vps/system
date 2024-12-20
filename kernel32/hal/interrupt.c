#include "interrupt.h"
#include "it/it.h"
#include "ke/core/panic.h"
#include "ke/core/mutex.h"
#include "mm/heap.h"
#include "arch.h"
#include "rtl/string.h"

/**
 * @brief Register architecture-specific IRQ
 * @param input IRQ number
 * @param vector Corresponding vector
 * @param params IRQ params
 * @return Status code
 */
STATUS HalArchRegiserIrq(uint32_t input, uint8_t vector, struct HalInterruptParams params);

/**
 * @brief Unregister architecture-specific IRQ
 * @param input IRQ number
 * @return Status code
 */
STATUS HalArchUnregisterIrq(uint32_t input);

/**
 * @brief Enable architecture-specific IRQ
 * @param input IRQ number
 * @return Status code
 */
STATUS HalArchEnableIrq(uint32_t input);

/**
 * @brief Disable architecture-specific IRQ
 * @param input IRQ number
 * @return Status code
 */
STATUS HalArchDisableIrq(uint32_t input);

struct HalInterruptEntry
{
    uint32_t input;
    uint8_t vector;
    struct HalInterruptParams params;
    uint8_t consumers;

    struct HalInterruptEntry *next;
} ;
static struct HalInterruptEntry *HalInterruptList = NULL;
static KeSpinlock HalInterruptListLock = KeSpinlockInitializer;



STATUS HalRegisterIrq(
    uint32_t input,
    ItHandler isr,
    void *context,
    struct HalInterruptParams params)
{
    STATUS status = OK;
    uint8_t vector = 0;

    struct HalInterruptEntry *matching = NULL;
    PRIO prio = KeAcquireSpinlock(&HalInterruptListLock);
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
            && (HAL_IT_SHAREABLE == matching->params.shared)
            && (HAL_IT_SHAREABLE == params.shared)
            ))
        {
            KeReleaseSpinlock(&HalInterruptListLock, prio);
            return INTERRUPT_ALREADY_REGISTERED;
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
            KeReleaseSpinlock(&HalInterruptListLock, prio);
            return OUT_OF_RESOURCES;
        }
        RtlMemset(matching, 0, sizeof(*matching));

        vector = ItReserveVector(HalIrqIsVectorRelatedToIrq() ? HalIrqVectorFromIrq(input) : IT_VECTOR_ANY);
        if(0 == vector)
        {
            MmFreeKernelHeap(matching);
            KeReleaseSpinlock(&HalInterruptListLock, prio);
            return OUT_OF_RESOURCES;
        }
        status = HalArchRegiserIrq(input, vector, params);
        if(OK != status)
        {
            ItFreeVector(vector);
            MmFreeKernelHeap(matching);
            KeReleaseSpinlock(&HalInterruptListLock, prio);
            return status; 
        }
        status = ItInstallInterruptHandler(vector, isr, context);
        if(OK != status)
        {
            HalArchUnregisterIrq(input);
            ItFreeVector(vector);
            MmFreeKernelHeap(matching);
            KeReleaseSpinlock(&HalInterruptListLock, prio);
            return status;
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

    KeReleaseSpinlock(&HalInterruptListLock, prio);
    return OK;
}

STATUS HalUnregisterIrq(uint32_t input, ItHandler isr)
{
    STATUS status = OK;

    struct HalInterruptEntry *matching = NULL;
    struct HalInterruptEntry *previous = NULL;
    PRIO prio = KeAcquireSpinlock(&HalInterruptListLock);
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
            status = HalArchUnregisterIrq(matching->input);
            if(OK != status)
                goto HalUnregisterIrqExit;

            if(NULL != previous)
                previous->next = matching->next;
            else
                HalInterruptList = matching->next;
            
            MmFreeKernelHeap(matching);
        }
    }
    else
    {
        status = INTERRUPT_NOT_REGISTERED;
    }
HalUnregisterIrqExit:
    KeReleaseSpinlock(&HalInterruptListLock, prio);
    return status;
}

uint8_t HalGetAssociatedVector(uint32_t input)
{
    uint8_t vector = 0;
    PRIO prio = KeAcquireSpinlock(&HalInterruptListLock);
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
    KeReleaseSpinlock(&HalInterruptListLock, prio);
    return vector;
}

STATUS HalEnableIrq(uint32_t input, ItHandler isr)
{
    STATUS status = OK;
    PRIO prio = KeAcquireSpinlock(&HalInterruptListLock);
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
        status = HalArchEnableIrq(input);

        if(OK == status)
        {
            status = ItSetInterruptHandlerEnable(t->vector, isr, true);
        }
    }
    else
        status = INTERRUPT_NOT_REGISTERED;

    KeReleaseSpinlock(&HalInterruptListLock, prio);
    return status;
}

STATUS HalDisableIrq(uint32_t input, ItHandler isr)
{
    STATUS status = OK;
    PRIO prio = KeAcquireSpinlock(&HalInterruptListLock);
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
        status = HalArchDisableIrq(input);

        if(OK == status)
        {
            status = ItSetInterruptHandlerEnable(t->vector, isr, false);
        }
    }
    else
        status = INTERRUPT_NOT_REGISTERED;

    KeReleaseSpinlock(&HalInterruptListLock, prio);
    return status;
}

PRIO HalRaisePriorityLevel(PRIO prio)
{
    if(prio > HAL_PRIORITY_LEVEL_HIGHEST)
        KePanicIPEx(KE_GET_CALLER_ADDRESS(0), ILLEGAL_PRIORITY_LEVEL, prio, 0, 0, 0);
    PRIO old = HalGetTaskPriority();
    barrier();
    if(prio < HalGetProcessorPriority())
        KePanicIPEx(KE_GET_CALLER_ADDRESS(0), ILLEGAL_PRIORITY_LEVEL_CHANGE, prio, HalGetProcessorPriority(), 0, 0);
    HalSetTaskPriority(prio);
    return old;
}

void HalLowerPriorityLevel(PRIO prio)
{
    if(prio > HAL_PRIORITY_LEVEL_HIGHEST)
        KePanicIPEx(KE_GET_CALLER_ADDRESS(0), ILLEGAL_PRIORITY_LEVEL, prio, 0, 0, 0);
    PRIO old = HalGetProcessorPriority();
    barrier();
    if(prio > old)
        KePanicIPEx(KE_GET_CALLER_ADDRESS(0), ILLEGAL_PRIORITY_LEVEL_CHANGE, prio, old, 0, 0);
    HalSetTaskPriority(prio);
}


void HalCheckPriorityLevel(PRIO lower, PRIO upper)
{
    PRIO current = HalGetProcessorPriority();
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wframe-address"
    if(current < lower)
        KePanicIPEx(KE_GET_CALLER_ADDRESS(1), PRIORITY_LEVEL_TOO_LOW, current, lower, 0, 0);
    if(current > upper)
        KePanicIPEx(KE_GET_CALLER_ADDRESS(1), PRIORITY_LEVEL_TOO_HIGH, current, upper, 0, 0);
#pragma GCC diagnostic pop
}