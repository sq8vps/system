#include "gdt.h"
#include "common.h"
#include "config.h"

#define GDT_MAX_ENTRIES (4 + MAX_CPU_COUNT)
#define GDT_PRESENT_FLAG (1 << 7)
#define GDT_PRIVILEGE_LEVEL_0 (0)
#define GDT_PRIVILEGE_LEVEL_3 ((1 << 6) | (1 << 5))
#define GDT_DATA_CODE_FLAG (1 << 4)
#define GDT_SYSTEM_FLAG (0)
#define GDT_EXECUTABLE_FLAG (1 << 3)
#define GDT_RW_FLAG (1 << 1)

#define GDT_32BIT_TSS 0x9

#define GDT_GRANURALITY_FLAG (1 << 7)
#define GDT_PROTECTED_MODE_FLAG (1 << 6)

/**
 * @brief A structure for generic GDT entry
*/
struct GdtEntry
{
    uint16_t limit1;
    uint16_t base1;
    uint8_t base2;
    uint8_t accessByte;
    uint8_t flagsAndLimit2;
    uint8_t base3;
} PACKED;

struct GdtEntry I686Gdt[GDT_MAX_ENTRIES + 1] ALIGN(8); //table of GDTs

/**
 * @brief A GDTR structure
*/
struct
{
    uint16_t size;
    uintptr_t offset;
} PACKED I686GdtRegister;

struct TssEntry 
{
    uint16_t link;
    uint16_t rsvd1;

    uint32_t esp0;
    uint16_t ss0;
    uint16_t rsvd2;

    uint32_t esp1;
    uint16_t ss1;
    uint16_t rsvd3;

    uint32_t esp2;
    uint16_t ss2;
    uint16_t rsvd4;

    uint32_t cr3;
    uint32_t eip;
    uint32_t eflags;
    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;

    uint16_t es;
    uint16_t rsvd5;
    uint16_t cs;
    uint16_t rsvd6;
    uint16_t ss;
    uint16_t rsvd7;
    uint16_t ds;
    uint16_t rsvd8;
    uint16_t fs;
    uint16_t rsvd9;
    uint16_t gs;
    uint16_t rsvd10;
    uint16_t ldtr;
    uint16_t rsvd11;

    uint16_t rsvd12;
    uint16_t iopb;

    uint32_t ssp;
} PACKED;

static struct TssEntry I686Tss[MAX_CPU_COUNT];

void GdtInit(void)
{
    CmMemset(I686Gdt, 0, sizeof(I686Gdt)); //clear all GDT entries
    CmMemset(I686Tss, 0, sizeof(I686Tss));
    I686Gdt[GDT_KERNEL_CS].limit1 = 0xFFFF;
    I686Gdt[GDT_KERNEL_CS].accessByte = GDT_PRESENT_FLAG | GDT_PRIVILEGE_LEVEL_0 | GDT_DATA_CODE_FLAG | GDT_EXECUTABLE_FLAG | GDT_RW_FLAG;
    I686Gdt[GDT_KERNEL_CS].flagsAndLimit2 = 0xF | GDT_GRANURALITY_FLAG | GDT_PROTECTED_MODE_FLAG;
    I686Gdt[GDT_KERNEL_DS].limit1 = 0xFFFF;
    I686Gdt[GDT_KERNEL_DS].accessByte = GDT_PRESENT_FLAG | GDT_PRIVILEGE_LEVEL_0 | GDT_DATA_CODE_FLAG | GDT_RW_FLAG;
    I686Gdt[GDT_KERNEL_DS].flagsAndLimit2 = 0xF | GDT_GRANURALITY_FLAG | GDT_PROTECTED_MODE_FLAG;
    I686Gdt[GDT_USER_CS].limit1 = 0xFFFF;
    I686Gdt[GDT_USER_CS].accessByte = GDT_PRESENT_FLAG | GDT_PRIVILEGE_LEVEL_3 | GDT_DATA_CODE_FLAG | GDT_EXECUTABLE_FLAG | GDT_RW_FLAG;
    I686Gdt[GDT_USER_CS].flagsAndLimit2 = 0xF | GDT_GRANURALITY_FLAG | GDT_PROTECTED_MODE_FLAG;
    I686Gdt[GDT_USER_DS].limit1 = 0xFFFF;
    I686Gdt[GDT_USER_DS].accessByte = GDT_PRESENT_FLAG | GDT_PRIVILEGE_LEVEL_3 | GDT_DATA_CODE_FLAG | GDT_RW_FLAG;
    I686Gdt[GDT_USER_DS].flagsAndLimit2 = 0xF | GDT_GRANURALITY_FLAG | GDT_PROTECTED_MODE_FLAG;

    I686GdtRegister.size = (sizeof(I686Gdt)) - 1;
    I686GdtRegister.offset = (uintptr_t)I686Gdt;

    GdtApply();
}

void GdtApply(void)
{
    ASM("lgdt %0" : : "m" (I686GdtRegister) : "memory"); //load new GDT register
    ASM("jmp %0:.1%=\n.1%=:" : : "X" (GDT_OFFSET(GDT_KERNEL_CS)) : "memory"); //perform far jump with code selector, CS can't be set directly
    ASM("mov ds,%0" : : "a" (GDT_OFFSET(GDT_KERNEL_DS)) : "memory"); //set up segment registers
    ASM("mov ss,%0" : : "a" (GDT_OFFSET(GDT_KERNEL_DS)) : "memory");
    ASM("mov es,%0" : : "a" (GDT_OFFSET(GDT_KERNEL_DS)) : "memory");
    ASM("mov fs,%0" : : "a" (GDT_OFFSET(GDT_KERNEL_DS)) : "memory");
    ASM("mov gs,%0" : : "a" (GDT_OFFSET(GDT_KERNEL_DS)) : "memory");
}

STATUS GdtAddCpu(uint16_t cpu)
{
    if(cpu >= MAX_CPU_COUNT)
        return MM_GDT_ENTRY_LIMIT_EXCEEDED;

    I686Tss[cpu].ss0 = GDT_OFFSET(GDT_KERNEL_DS);
    I686Tss[cpu].esp0 = 0;
    I686Tss[cpu].iopb = sizeof(struct TssEntry);

    I686Gdt[GDT_TSS(cpu)].limit1 = sizeof(struct TssEntry) - 1;
    I686Gdt[GDT_TSS(cpu)].base1 = (uintptr_t)(&I686Tss[cpu]) & (uintptr_t)0xFFFF;
    I686Gdt[GDT_TSS(cpu)].base2 = ((uintptr_t)(&I686Tss[cpu]) & (uintptr_t)0xFF0000) >> 16;
    I686Gdt[GDT_TSS(cpu)].base3 = ((uintptr_t)(&I686Tss[cpu]) & (uintptr_t)0xFF000000) >> 24;
    I686Gdt[GDT_TSS(cpu)].flagsAndLimit2 = 0;
    I686Gdt[GDT_TSS(cpu)].accessByte = GDT_PRESENT_FLAG | GDT_PRIVILEGE_LEVEL_0 | GDT_SYSTEM_FLAG | GDT_32BIT_TSS;

    return OK;
}

void GdtLoadTss(uint16_t cpu)
{
    //load GDT descriptor with TSS to task register
    ASM("ltr %0" : : "r" GDT_OFFSET(GDT_TSS(cpu)));
}

__attribute__((fastcall))
void GdtUpdateTss(uintptr_t esp0)
{
    uint16_t register t;
    //get GDT descriptor with TSS from task register
    ASM("str %0" : "=r" (t) :);
    //convert GDT descriptor offset to CPU number and update kernel stack pointer
    I686Tss[GDT_CPU(GDT_ENTRY(t))].esp0 = esp0;
}