#include "it.h"
#include "ke/core/mutex.h"
#include "hal/hal.h"
#include "hal/interrupt.h"
#include "common.h"
#include "mm/gdt.h"
#include "handlers/nmi.h"
#include "handlers/divzero.h"
#include "handlers/traps.h"
#include "handlers/bound.h"
#include "handlers/ud.h"
#include "handlers/fpufaults.h"
#include "handlers/dfault.h"
#include "handlers/gp.h"
#include "handlers/pagefault.h"
#include "wrappers.h"
#include "ke/core/dpc.h"

#define IDT_FLAG_INTERRUPT_GATE 0xE
#define IDT_FLAG_TRAP_GATE  0xF
#define IDT_FLAG_INTERRUPT_USERMODE 0x60
#define IDT_FLAG_PRESENT 0x80

#define IDT_ENTRY_COUNT 256

/**
 * @brief IDT entry structure
*/
struct IdtEntry
{
	uint16_t isrLow; //lower word of ISR address
	uint16_t selector; //GDT selector for code segment
	uint8_t reserved; //zeros
	uint8_t flags; //attributes
	uint16_t isrHigh; //higher word of ISR address
} PACKED;

/**
 * @brief IDT register
*/
struct
{
	uint16_t limit;
	uint32_t base;
} PACKED idtr;

/**
 * @brief Interrupt Descriptor Table itself
*/
static struct IdtEntry idt[IDT_ENTRY_COUNT] __attribute__((aligned(8)));
 
static KeSpinlock ItHandlerTableMutex = KeSpinlockInitializer;

uint8_t ItReserveVector(uint8_t vector)
{
	if(0 == vector)
	{
		KeAcquireSpinlock(&ItHandlerTableMutex);
		for(uint16_t i = 0; i < sizeof(itHandlerDescriptorTable) / sizeof(*itHandlerDescriptorTable); i++)
		{
			if((0 == itHandlerDescriptorTable[i].count) && (false == itHandlerDescriptorTable[i].reserved))
			{
				KeReleaseSpinlock(&ItHandlerTableMutex);
				return i + IT_FIRST_INTERRUPT_VECTOR;
			}
		}
		vector = 0;
	}
	else if(vector >= IT_FIRST_INTERRUPT_VECTOR)
	{
		vector -= IT_FIRST_INTERRUPT_VECTOR;
		KeAcquireSpinlock(&ItHandlerTableMutex);
		if(0 == itHandlerDescriptorTable[vector].count)
			itHandlerDescriptorTable[vector].reserved = true;
		else
			vector = 0;
	}
	KeReleaseSpinlock(&ItHandlerTableMutex);
	return vector;
}

void ItFreeVector(uint8_t vector)
{
	if(vector < IT_FIRST_INTERRUPT_VECTOR)
		return;
	
	vector -= IT_FIRST_INTERRUPT_VECTOR;

	KeAcquireSpinlock(&ItHandlerTableMutex);
	if(true == itHandlerDescriptorTable[vector].reserved)
	{
		itHandlerDescriptorTable[vector].reserved = false;
	}
	KeReleaseSpinlock(&ItHandlerTableMutex);
}

STATUS ItInstallInterruptHandler(uint8_t vector, ItHandler isr, void *context)
{
	if(vector < IT_FIRST_INTERRUPT_VECTOR)
		return IT_BAD_VECTOR;

	vector -= IT_FIRST_INTERRUPT_VECTOR;

	KeAcquireSpinlock(&ItHandlerTableMutex);

	if(itHandlerDescriptorTable[vector].count == IT_MAX_SHARED_IRQ_CONSUMERS)
	{
		KeReleaseSpinlock(&ItHandlerTableMutex);
		return IT_VECTOR_NOT_FREE;
	}

	itHandlerDescriptorTable[vector].consumer[itHandlerDescriptorTable[vector].count].callback = isr;
	itHandlerDescriptorTable[vector].consumer[itHandlerDescriptorTable[vector].count].context = context;
	itHandlerDescriptorTable[vector].count++;
	itHandlerDescriptorTable[vector].reserved = false;
	KeReleaseSpinlock(&ItHandlerTableMutex);

	return OK;
}

STATUS ItUninstallInterruptHandler(uint8_t vector, ItHandler isr)
{
	if(vector < IT_FIRST_INTERRUPT_VECTOR)
		return IT_BAD_VECTOR;

	vector -= IT_FIRST_INTERRUPT_VECTOR;
	
	KeAcquireSpinlock(&ItHandlerTableMutex);
	for(uint8_t i = 0; i < itHandlerDescriptorTable[vector].count; i++)
	{
		if(itHandlerDescriptorTable[vector].consumer[i].callback == isr)
		{
			for(uint8_t k = IT_MAX_SHARED_IRQ_CONSUMERS; k > i; k--)
			{
				if(k > 1)
					itHandlerDescriptorTable[vector].consumer[k - 2] = itHandlerDescriptorTable[vector].consumer[k - 1];
			}
			for(uint8_t k = i; k < IT_MAX_SHARED_IRQ_CONSUMERS; k++)
			{
				itHandlerDescriptorTable[vector].consumer[k].callback = NULL;
			}
			itHandlerDescriptorTable[vector].count--;
			KeReleaseSpinlock(&ItHandlerTableMutex);
			return OK;
		}
	}
	KeReleaseSpinlock(&ItHandlerTableMutex);
	return IT_NOT_REGISTERED;
}

STATUS ItSetInterruptHandlerEnable(uint8_t vector, ItHandler isr, bool enable)
{
	if(vector < IT_FIRST_INTERRUPT_VECTOR)
		return IT_BAD_VECTOR;

	vector -= IT_FIRST_INTERRUPT_VECTOR;
	
	KeAcquireSpinlock(&ItHandlerTableMutex);
	for(uint8_t i = 0; i < itHandlerDescriptorTable[vector].count; i++)
	{
		if(itHandlerDescriptorTable[vector].consumer[i].callback == isr)
		{
			itHandlerDescriptorTable[vector].consumer[i].enabled = enable;
			KeReleaseSpinlock(&ItHandlerTableMutex);
			return OK;
		}
	}
	KeReleaseSpinlock(&ItHandlerTableMutex);
	return IT_NOT_REGISTERED;	
}

static STATUS insertIdtEntry(uint8_t vector, void *wrapper)
{
	if(vector < IT_FIRST_INTERRUPT_VECTOR)
		return IT_NO_FREE_VECTORS;
	idt[vector].isrLow = (uint32_t)wrapper & 0xFFFF;
	idt[vector].isrHigh = (uint32_t)wrapper >> 16;
	idt[vector].selector = MmGdtGetFlatPrivilegedCodeOffset(); //interrupt are always processed in kernel mode
	idt[vector].flags = IDT_FLAG_INTERRUPT_GATE | IDT_FLAG_PRESENT;
	return OK;
}

static STATUS ItInstallExceptionHandler(enum ItExceptionVector vector, void *isr)
{
	if(vector >= IT_FIRST_INTERRUPT_VECTOR)
		return IT_BAD_VECTOR;
	
	idt[vector].isrLow = (uint32_t)isr & 0xFFFF;
	idt[vector].isrHigh = (uint32_t)isr >> 16;
	idt[vector].selector = MmGdtGetFlatPrivilegedCodeOffset();
	idt[vector].flags = IDT_FLAG_TRAP_GATE | IDT_FLAG_PRESENT;
	return OK;
}

STATUS ItInit(void)
{
	CmMemset(idt, 0, sizeof(idt));
	for(uint16_t i = 0; i < sizeof(itHandlerDescriptorTable) / sizeof(*itHandlerDescriptorTable); i++)
	{
		itHandlerDescriptorTable[i].count = 0;
		CmMemset(itHandlerDescriptorTable[i].consumer, 0, sizeof(itHandlerDescriptorTable[i].consumer[0]));
	}
	//set up all defined exceptions to default handlers
	ItInstallExceptionHandler(IT_EXCEPTION_DIVIDE, ItDivisionByZeroHandler);
	ItInstallExceptionHandler(IT_EXCEPTION_DEBUG, ItDebugHandler);
	ItInstallExceptionHandler(IT_EXCEPTION_NMI, ItNmiHandler);
	ItInstallExceptionHandler(IT_EXCEPTION_BREAKPOINT, ItBreakpointHandler);
	ItInstallExceptionHandler(IT_EXCEPTION_OVERFLOW, ItOverflowHandler);
	ItInstallExceptionHandler(IT_EXCEPTION_BOUND_EXCEEDED, ItBoundExceededHandler);
	ItInstallExceptionHandler(IT_EXCEPTION_INVALID_OPCODE, ItInvalidOpcodeHandler);
	ItInstallExceptionHandler(IT_EXCEPTION_DEVICE_UNAVAILABLE, ItUnexpectedFaultHandler);
	ItInstallExceptionHandler(IT_EXCEPTION_DOUBLE_FAULT, ItDoubleFaultHandler);
	ItInstallExceptionHandler(IT_EXCEPTION_COPROCESSOR_OVERRUN, ItUnexpectedFaultHandler);
	ItInstallExceptionHandler(IT_EXCEPTION_INVALID_TSS, ItUnexpectedFaultHandlerEC);
	ItInstallExceptionHandler(IT_EXCEPTION_SEGMENT_NOT_PRESENT, ItUnexpectedFaultHandlerEC); 
	ItInstallExceptionHandler(IT_EXCEPTION_STACK_FAULT, ItUnexpectedFaultHandlerEC);
	ItInstallExceptionHandler(IT_EXCEPTION_GENERAL_PROTECTION, ItGeneralProtectionHandler);
	ItInstallExceptionHandler(IT_EXCEPTION_PAGE_FAULT, ItPageFaultHandler);
	ItInstallExceptionHandler(IT_EXCEPTION_FPU_ERROR, ItUnexpectedFaultHandler);
	ItInstallExceptionHandler(IT_EXCEPTION_ALIGNMENT_CHECK, ItUnexpectedFaultHandlerEC);
	ItInstallExceptionHandler(IT_EXCEPTION_MACHINE_CHECK, ItMachineCheckHandler);
	ItInstallExceptionHandler(IT_EXCEPTION_SIMD_FPU, ItSimdFpuHandler);
	ItInstallExceptionHandler(IT_EXCEPTION_VIRTUALIZATION, ItUnexpectedFaultHandler);
	ItInstallExceptionHandler(IT_EXCEPTION_CONTROL_PROTECTION, ItUnexpectedFaultHandlerEC);

	//install dummy handlers for all non-exception interrupts
	for(uint16_t i = IT_FIRST_INTERRUPT_VECTOR; i < IDT_ENTRY_COUNT; i++)
	{
		insertIdtEntry(i, itWrappers[i - IT_FIRST_INTERRUPT_VECTOR]);
	}

	//fill IDT register with IDT address and size
	idtr.base = (uintptr_t)idt;
	idtr.limit = sizeof(struct IdtEntry) * IDT_ENTRY_COUNT - 1;
	//load IDT register address
	ASM("lidt %0" : : "m" (idtr));
	//enable interrupts
	ItEnableInterrupts();

	return OK;
}

bool ItIsCausedByKernelMode(uint32_t cs)
{
	return (MmGdtGetFlatPrivilegedCodeOffset() == cs);
}

NORETURN void ItHardReset(void)
{
	idtr.limit = 0;
	ASM("lidt %0" : : "m" (idtr));
	ItEnableInterrupts();
	ASM("int 0");
	while(1)
		;
}

void ItDisableInterrupts(void)
{
	ASM("cli");
}

void ItEnableInterrupts(void)
{
	ASM("sti");
}

void ItHandleIrq(uint8_t vector)
{
	if(HalIsInterruptSpurious())                                               
        return;            
	ItEnableInterrupts();
	for(uint8_t i = 0; i < itHandlerDescriptorTable[vector - IT_FIRST_INTERRUPT_VECTOR].count; i++)        
	{                                
		if(itHandlerDescriptorTable[vector - IT_FIRST_INTERRUPT_VECTOR].consumer[i].enabled)              
			itHandlerDescriptorTable[vector - IT_FIRST_INTERRUPT_VECTOR].consumer[i].callback(itHandlerDescriptorTable[vector - IT_FIRST_INTERRUPT_VECTOR].consumer[i].context);
	}
	HalClearInterruptFlag(vector);
	KeProcessDpcQueue();
}