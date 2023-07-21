#include "it.h"
#include "ke/mutex.h"
#include "hal/hal.h"
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

/**
 * @brief Bitmap of used IRQ vectors
*/
static uint32_t irqUsageBitmap[IDT_ENTRY_COUNT / 32];
 
/**
 * @brief Default ISR for interrupts
*/
IT_HANDLER void defaultIsr(struct ItFrame *f)
{

}

static KeSpinLock_t getFreeVectorMutex;

uint8_t ItGetFreeVector(void)
{
	KeAcquireSpinlockDisableIRQ(&getFreeVectorMutex);
	for(uint8_t i = 1; i < sizeof(irqUsageBitmap); i++)
	{
		for(uint8_t k = 0; k < 32; k++)
		{
			if(0 == (irqUsageBitmap[i] & ((uint32_t)1 << k)))
			{
				irqUsageBitmap[i] |= ((uint32_t)1 << k);
				KeReleaseSpinlockEnableIRQ(&getFreeVectorMutex);
				return (i * 32) + k;
			}
		} 	
	}
	KeReleaseSpinlockEnableIRQ(&getFreeVectorMutex);
	return 0;
}

STATUS ItInstallInterruptHandler(uint8_t vector, void *isr, PrivilegeLevel_t cpl)
{
	if(vector < IT_FIRST_INTERRUPT_VECTOR)
		return IT_NO_FREE_VECTORS;
	idt[vector].isrLow = (uint32_t)isr & 0xFFFF;
	idt[vector].isrHigh = (uint32_t)isr >> 16;
	idt[vector].selector = MmGdtGetFlatPrivilegedCodeOffset(); //interrupt are always processed in kernel mode
	idt[vector].flags = IDT_FLAG_INTERRUPT_GATE | IDT_FLAG_PRESENT | ((PL_USER == cpl) ? IDT_FLAG_INTERRUPT_USERMODE : 0);
	return OK;
}


STATUS ItUninstallInterruptHandler(uint8_t vector)
{
	if(vector < IT_FIRST_INTERRUPT_VECTOR)
		return IT_BAD_VECTOR;
	
	ItInstallInterruptHandler(vector, defaultIsr, PL_KERNEL);

	irqUsageBitmap[vector / 32] &= ~((uint32_t)1 << (vector % 32));
	
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
	//set up all defined exceptions to default handlers
	ItInstallExceptionHandler(IT_EXCEPTION_DIVIDE, ItDivisionByZeroHandler);
	ItInstallExceptionHandler(IT_EXCEPTION_DEBUG, ItDebugHandler);
	ItInstallExceptionHandler(IT_EXCEPTION_NMI, ItNmiHandler);
	ItInstallExceptionHandler(IT_EXCEPTION_BREAKPOINT, ItBreakpointHandler);
	ItInstallExceptionHandler(IT_EXCEPTION_OVERFLOW, ItOverflowHandler);
	ItInstallExceptionHandler(IT_EXCEPTION_BOUND_EXCEEDED, ItBoundExceededHandler);
	ItInstallExceptionHandler(IT_EXCEPTION_INVALID_OPCODE, ItInvalidOpcodeHandler);
	ItInstallExceptionHandler(IT_EXCEPTION_DEVICE_UNAVAILABLE, ItFpuUnavailableHandler);
	ItInstallExceptionHandler(IT_EXCEPTION_DOUBLE_FAULT, ItDoubleFaultHandler);
	ItInstallExceptionHandler(IT_EXCEPTION_COPROCESSOR_OVERRUN, ItUnexpectedFaultHandler);
	ItInstallExceptionHandler(IT_EXCEPTION_INVALID_TSS, ItUnexpectedFaultHandlerEC);
	ItInstallExceptionHandler(IT_EXCEPTION_SEGMENT_NOT_PRESENT, ItUnexpectedFaultHandlerEC); 
	ItInstallExceptionHandler(IT_EXCEPTION_STACK_FAULT, ItUnexpectedFaultHandlerEC);
	ItInstallExceptionHandler(IT_EXCEPTION_GENERAL_PROTECTION, ItGeneralProtectionHandler);
	ItInstallExceptionHandler(IT_EXCEPTION_PAGE_FAULT, ItPAgeFaultHandler);
	ItInstallExceptionHandler(IT_EXCEPTION_FPU_ERROR, ItUnexpectedFaultHandler);
	ItInstallExceptionHandler(IT_EXCEPTION_ALIGNMENT_CHECK, ItUnexpectedFaultHandlerEC);
	ItInstallExceptionHandler(IT_EXCEPTION_MACHINE_CHECK, ItMachineCheckHandler);
	ItInstallExceptionHandler(IT_EXCEPTION_SIMD_FPU, ItSimdFpuHandler);
	ItInstallExceptionHandler(IT_EXCEPTION_VIRTUALIZATION, ItUnexpectedFaultHandler);
	ItInstallExceptionHandler(IT_EXCEPTION_CONTROL_PROTECTION, ItUnexpectedFaultHandlerEC);

	irqUsageBitmap[0] = 0xFFFFFFFF; //mark first 32 vectors as used

	//install dummy handlers for all non-exception interrupts
	for(uint16_t i = IT_FIRST_INTERRUPT_VECTOR; i < IDT_ENTRY_COUNT; i++)
	{
		ItInstallInterruptHandler(i, defaultIsr, PL_KERNEL);
	}

	//fill IDT register with IDT address and size
	idtr.base = (uintptr_t)idt;
	idtr.limit = sizeof(struct IdtEntry) * IDT_ENTRY_COUNT - 1;
	//load IDT register address
	ASM("lidt %0" : : "m" (idtr));
	//enable interrupts
	ASM("sti");

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
	ASM("sti");
	ASM("int 0");
	while(1)
		;
}

