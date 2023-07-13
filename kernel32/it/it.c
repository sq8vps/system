#include "it.h"
#include "../../drivers/vga/vga.h"
#include "../hal/hal.h"
#include "../common.h"
#include "../mm/gdt.h"

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
	uint8_t unused; //zeros
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
 * @brief Default ISR for interrupts/exceptions with no error code
*/
IT_HANDLER void defaultIt_h(struct ItFrame *f)
{
	printf("Unhandled interrupt! EIP: 0x%X, CS: 0x%X, flags: 0x%X\n", f->ip, f->cs, f->flags);
	// asm volatile("xchg bx, bx");
	// asm volatile("cli");
	// asm volatile("hlt");
	while(1);;
}

/**
 * @brief Default ISR for exceptions with error code
*/
IT_HANDLER void defaultItEC_h(struct ItFrameEC *f)
{
	printf("Unhandled exception! Error code: 0x%X, EIP: 0x%X, CS: 0x%X, flags: 0x%X\n", f->error, f->ip, f->cs, f->flags);
	// asm volatile("xchg bx, bx");
	// asm volatile("cli");
	// asm volatile("hlt");
	while(1);;
}

STATUS It_setInterruptHandler(uint8_t vector, void *isr, PrivilegeLevel_t cpl)
{
	if(vector < IT_FIRST_INTERRUPT_VECTOR)
		return IT_BAD_VECTOR;
	
	idt[vector].isrLow = (uint32_t)isr & 0xFFFF;
	idt[vector].isrHigh = (uint32_t)isr >> 16;
	idt[vector].selector = MmGdtGetFlatPrivilegedCodeOffset();
	idt[vector].flags = IDT_FLAG_INTERRUPT_GATE | IDT_FLAG_PRESENT | ((PL_USER == cpl) ? IDT_FLAG_INTERRUPT_USERMODE : 0);
	return OK;
}

STATUS It_setExceptionHandler(enum It_exceptionVector vector, void *isr)
{
	if(vector >= IT_FIRST_INTERRUPT_VECTOR)
		return IT_BAD_VECTOR;
	
	idt[vector].isrLow = (uint32_t)isr & 0xFFFF;
	idt[vector].isrHigh = (uint32_t)isr >> 16;
	idt[vector].selector = MmGdtGetFlatPrivilegedCodeOffset();
	idt[vector].flags = IDT_FLAG_TRAP_GATE | IDT_FLAG_PRESENT;
	return OK;
}

STATUS It_init(void)
{
	Cm_memset(idt, 0, sizeof(idt));
	//set up all defined exceptions to default handlers
	It_setExceptionHandler(IT_EXCEPTION_DIVIDE, defaultIt_h);
	It_setExceptionHandler(IT_EXCEPTION_DEBUG, defaultIt_h);
	It_setExceptionHandler(IT_EXCEPTION_NMI, defaultIt_h);
	It_setExceptionHandler(IT_EXCEPTION_BREAKPOINT, defaultIt_h);
	It_setExceptionHandler(IT_EXCEPTION_OVERFLOW, defaultIt_h);
	It_setExceptionHandler(IT_EXCEPTION_BOUND_EXCEEDED, defaultIt_h);
	It_setExceptionHandler(IT_EXCEPTION_IVALID_OPCODE, defaultIt_h);
	It_setExceptionHandler(IT_EXCEPTION_DEVICE_UNAVAILABLE, defaultIt_h);
	It_setExceptionHandler(IT_EXCEPTION_DOUBLE_FAULT, defaultIt_h);
	It_setExceptionHandler(IT_EXCEPTION_COPROCESSOR_OVERRUN, defaultIt_h);
	It_setExceptionHandler(IT_EXCEPTION_INVALID_TSS, defaultItEC_h);
	It_setExceptionHandler(IT_EXCEPTION_SEGMENT_NOT_PRESENT, defaultItEC_h); 
	It_setExceptionHandler(IT_EXCEPTION_STACK_FAULT, defaultItEC_h);
	It_setExceptionHandler(IT_EXCEPTION_GENERAL_PROTECTION, defaultItEC_h);
	It_setExceptionHandler(IT_EXCEPTION_PAGE_FAULT, defaultItEC_h);
	It_setExceptionHandler(IT_EXCEPTION_FPU_ERROR, defaultIt_h);
	It_setExceptionHandler(IT_EXCEPTION_ALIGNMENT_CHECK, defaultItEC_h);
	It_setExceptionHandler(IT_EXCEPTION_MACHINE_CHECK, defaultIt_h);
	It_setExceptionHandler(IT_EXCEPTION_DIVIDE, defaultIt_h);
	It_setExceptionHandler(IT_EXCEPTION_SIMD_FPU, defaultIt_h);
	It_setExceptionHandler(IT_EXCEPTION_VIRTUALIZATION, defaultIt_h);
	It_setExceptionHandler(IT_EXCEPTION_CONTROL_PROTECTION, defaultItEC_h);

	//fill IDT register with IDT address and size
	idtr.base = (uintptr_t)&idt[0];
	idtr.limit = sizeof(struct IdtEntry) * IDT_ENTRY_COUNT - 1;
	//load IDT register address
	asm volatile("lidt %0" : : "m" (idtr));
	//enable interrupts
	asm volatile ("sti");

	return OK;
}





