#include "it.h"
#include "defines.h"

#define GDT_CODE_SELECTOR 0x08

#define IDT_FLAG_INTERRUPT_GATE 0xE
#define IDT_FLAG_TRAP_GATE  0xF
#define IDT_FLAG_INTERRUPT_USERMODE 0x60
#define IDT_FLAG_PRESENT 0x80

struct ItFrame
{
    uint32_t ip;
    uint32_t cs;
    uint32_t flags;
} __attribute__((packed));

struct ItFrameEC
{
    uint32_t error;
	uint32_t ip;
    uint32_t cs;
    uint32_t flags;
} __attribute__((packed));

struct IdtEntry
{
	uint16_t isrLow; //lower word of ISR address
	uint16_t selector; //GDT selector for code segment
	uint8_t unused; //zeros
	uint8_t flags; //attributes
	uint16_t isrHigh; //higher word of ISR address
} __attribute__((packed));

struct
{
	uint16_t limit;
	uint32_t base;
} __attribute__((packed)) idtr;

static struct IdtEntry idt[256] __attribute__((aligned(8)));

__attribute__ ((interrupt))
void defaultIt_h(struct ItFrame *f)
{
	asm volatile ("xchg bx,bx");
}

__attribute__ ((interrupt))
void defaultItEC_h(struct ItFrameEC *f)
{
	asm volatile("xchg bx, bx");

}

void It_setInterruptHandler(uint8_t vector, void *isr, bool_t kernelModeOnly)
{
	idt[vector].isrLow = (uint32_t)isr & 0xFFFF;
	idt[vector].isrHigh = (uint32_t)isr >> 16;
	idt[vector].selector = GDT_CODE_SELECTOR;
	idt[vector].flags = IDT_FLAG_INTERRUPT_GATE | IDT_FLAG_PRESENT | ((kernelModeOnly == false) ? IDT_FLAG_INTERRUPT_USERMODE : 0);
}

static void it_setExceptionHandler(uint8_t vector, void *isr)
{
	idt[vector].isrLow = (uint32_t)isr & 0xFFFF;
	idt[vector].isrHigh = (uint32_t)isr >> 16;
	idt[vector].selector = GDT_CODE_SELECTOR;
	idt[vector].flags = IDT_FLAG_TRAP_GATE | IDT_FLAG_PRESENT;
}

void It_init(void)
{
	it_setExceptionHandler(0, defaultIt_h);
	for(uint16_t i = 32; i < 256; i++)
	{
		It_setInterruptHandler(i, defaultIt_h, false);
	}
}







