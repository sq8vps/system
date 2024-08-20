#include "it.h"
#include "ke/core/mutex.h"
#include "hal/hal.h"
#include "hal/interrupt.h"
#include "common.h"
#include "wrappers.h"
#include "ke/core/dpc.h"
#include "hal/arch.h"
 
/**
 * @brief Interrupt handler descriptor
*/
struct ItHandlerDescriptor
{
    bool reserved; /**< Vector is not used, but reserved for IRQ registering */
    uint8_t count; /**< Number of interrupt consumers */
    struct
    {
        bool enabled; /**< Consumer enabled */
        ItHandler callback; /**< Consumer ISR */
        void *context; /**< ISR's context */
    } consumer[IT_MAX_SHARED_IRQ_CONSUMERS]; /**< A table of interrupt consumers */
};

/**
 * @brief Table of interrupt descriptors
 */
static struct ItHandlerDescriptor ItHandlerDescriptorTable[IT_HANDLER_COUNT];

static KeSpinlock ItHandlerTableMutex = KeSpinlockInitializer;

uint8_t ItReserveVector(uint8_t vector)
{
	PRIO prio = KeAcquireSpinlock(&ItHandlerTableMutex);
	if(0 == vector)
	{
		for(uint16_t i = 0; i < sizeof(ItHandlerDescriptorTable) / sizeof(*ItHandlerDescriptorTable); i++)
		{
			if((0 == ItHandlerDescriptorTable[i].count) && (false == ItHandlerDescriptorTable[i].reserved))
			{
				KeReleaseSpinlock(&ItHandlerTableMutex, prio);
				return i + IT_FIRST_INTERRUPT_VECTOR;
			}
		}
		vector = 0;
	}
	else if(vector >= IT_FIRST_INTERRUPT_VECTOR)
	{
		if(0 == ItHandlerDescriptorTable[vector - IT_FIRST_INTERRUPT_VECTOR].count)
			ItHandlerDescriptorTable[vector - IT_FIRST_INTERRUPT_VECTOR].reserved = true;
		else
			vector = 0;
	}
	else
		vector = 0;
	KeReleaseSpinlock(&ItHandlerTableMutex, prio);
	return vector;
}

void ItFreeVector(uint8_t vector)
{
	if(vector < IT_FIRST_INTERRUPT_VECTOR)
		return;
	
	vector -= IT_FIRST_INTERRUPT_VECTOR;

	PRIO prio = KeAcquireSpinlock(&ItHandlerTableMutex);
	if(true == ItHandlerDescriptorTable[vector].reserved)
	{
		ItHandlerDescriptorTable[vector].reserved = false;
	}
	KeReleaseSpinlock(&ItHandlerTableMutex, prio);
}

STATUS ItInstallInterruptHandler(uint8_t vector, ItHandler isr, void *context)
{
	if(vector < IT_FIRST_INTERRUPT_VECTOR)
		return IT_BAD_VECTOR;

	vector -= IT_FIRST_INTERRUPT_VECTOR;

	PRIO prio = KeAcquireSpinlock(&ItHandlerTableMutex);

	if(ItHandlerDescriptorTable[vector].count == IT_MAX_SHARED_IRQ_CONSUMERS)
	{
		KeReleaseSpinlock(&ItHandlerTableMutex, prio);
		return IT_VECTOR_NOT_FREE;
	}

	ItHandlerDescriptorTable[vector].consumer[ItHandlerDescriptorTable[vector].count].callback = isr;
	ItHandlerDescriptorTable[vector].consumer[ItHandlerDescriptorTable[vector].count].context = context;
	ItHandlerDescriptorTable[vector].count++;
	ItHandlerDescriptorTable[vector].reserved = false;
	KeReleaseSpinlock(&ItHandlerTableMutex, prio);

	return OK;
}

STATUS ItUninstallInterruptHandler(uint8_t vector, ItHandler isr)
{
	if(vector < IT_FIRST_INTERRUPT_VECTOR)
		return IT_BAD_VECTOR;

	vector -= IT_FIRST_INTERRUPT_VECTOR;
	
	PRIO prio = KeAcquireSpinlock(&ItHandlerTableMutex);
	for(uint8_t i = 0; i < ItHandlerDescriptorTable[vector].count; i++)
	{
		if(ItHandlerDescriptorTable[vector].consumer[i].callback == isr)
		{
			for(uint8_t k = IT_MAX_SHARED_IRQ_CONSUMERS; k > i; k--)
			{
				if(k > 1)
					ItHandlerDescriptorTable[vector].consumer[k - 2] = ItHandlerDescriptorTable[vector].consumer[k - 1];
			}
			for(uint8_t k = i; k < IT_MAX_SHARED_IRQ_CONSUMERS; k++)
			{
				ItHandlerDescriptorTable[vector].consumer[k].callback = NULL;
			}
			ItHandlerDescriptorTable[vector].count--;
			KeReleaseSpinlock(&ItHandlerTableMutex, prio);
			return OK;
		}
	}
	KeReleaseSpinlock(&ItHandlerTableMutex, prio);
	return IT_NOT_REGISTERED;
}

STATUS ItSetInterruptHandlerEnable(uint8_t vector, ItHandler isr, bool enable)
{
	if(vector < IT_FIRST_INTERRUPT_VECTOR)
		return IT_BAD_VECTOR;

	vector -= IT_FIRST_INTERRUPT_VECTOR;
	
	PRIO prio = KeAcquireSpinlock(&ItHandlerTableMutex);
	for(uint8_t i = 0; i < ItHandlerDescriptorTable[vector].count; i++)
	{
		if(ItHandlerDescriptorTable[vector].consumer[i].callback == isr)
		{
			ItHandlerDescriptorTable[vector].consumer[i].enabled = enable;
			KeReleaseSpinlock(&ItHandlerTableMutex, prio);
			return OK;
		}
	}
	KeReleaseSpinlock(&ItHandlerTableMutex, prio);
	return IT_NOT_REGISTERED;	
}

STATUS ItInit(void)
{
	for(uint16_t i = 0; i < sizeof(ItHandlerDescriptorTable) / sizeof(*ItHandlerDescriptorTable); i++)
	{
		ItHandlerDescriptorTable[i].count = 0;
		CmMemset(ItHandlerDescriptorTable[i].consumer, 0, sizeof(ItHandlerDescriptorTable[i].consumer[0]));
	}
	return OK;
}

void ItHandleIrq(uint8_t vector)
{
	HalEnableInterrupts();
	if(HalIsInterruptSpurious())                                               
        return;            
	for(uint8_t i = 0; i < ItHandlerDescriptorTable[vector - IT_FIRST_INTERRUPT_VECTOR].count; i++)        
	{                                
		if(ItHandlerDescriptorTable[vector - IT_FIRST_INTERRUPT_VECTOR].consumer[i].enabled)              
			ItHandlerDescriptorTable[vector - IT_FIRST_INTERRUPT_VECTOR].consumer[i].callback(ItHandlerDescriptorTable[vector - IT_FIRST_INTERRUPT_VECTOR].consumer[i].context);
	}
	HalClearInterruptFlag(vector);
	KeProcessDpcQueue();
}