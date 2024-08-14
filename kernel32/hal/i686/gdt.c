#include "gdt.h"
#include "common.h"
#include "config.h"

#define GDT_MAX_ENTRIES (4 + MAX_CPU_COUNT)
#define GDT_PRESENT_FLAG 128
#define GDT_PRIVILEGE_LEVEL_0 0
#define GDT_PRIVILEGE_LEVEL_3 (64 | 32)
#define GDT_CODE_FLAG 16
#define GDT_DATA_FLAG 16
#define GDT_EXECUTABLE_FLAG 8
#define GDT_READABLE_FLAG 2
#define GDT_WRITABLE_FLAG 2

#define GDT_32BIT_TSS_FLAG 9

#define GDT_PAGE_GRANURALITY 128
#define GDT_PROTECTED_MODE_FLAG 64
#define GDT_LONG_MODE_FLAG 32

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

static struct GdtEntry gdt[GDT_MAX_ENTRIES + 1]; //table of GDTs

/**
 * @brief A GDTR structure
*/
struct GdtRegister
{
    uint16_t size;
    uintptr_t offset;
} PACKED static gdtr;

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

static struct TssEntry tss[MAX_CPU_COUNT];

void GdtInit(void)
{
    CmMemset(gdt, 0, sizeof(gdt)); //clear all GDT entries
    CmMemset(tss, 0, sizeof(tss));
    gdt[GDT_KERNEL_CS].limit1 = 0xFFFF;
    gdt[GDT_KERNEL_CS].accessByte = GDT_PRESENT_FLAG | GDT_PRIVILEGE_LEVEL_0 | GDT_CODE_FLAG | GDT_EXECUTABLE_FLAG | GDT_READABLE_FLAG;
    gdt[GDT_KERNEL_CS].flagsAndLimit2 = 0xF | GDT_PAGE_GRANURALITY | GDT_PROTECTED_MODE_FLAG;
    gdt[GDT_KERNEL_DS].limit1 = 0xFFFF;
    gdt[GDT_KERNEL_DS].accessByte = GDT_PRESENT_FLAG | GDT_PRIVILEGE_LEVEL_0 | GDT_DATA_FLAG | GDT_WRITABLE_FLAG;
    gdt[GDT_KERNEL_DS].flagsAndLimit2 = 0xF | GDT_PAGE_GRANURALITY | GDT_PROTECTED_MODE_FLAG;
    gdt[GDT_USER_CS].limit1 = 0xFFFF;
    gdt[GDT_USER_CS].accessByte = GDT_PRESENT_FLAG | GDT_PRIVILEGE_LEVEL_3 | GDT_CODE_FLAG | GDT_EXECUTABLE_FLAG | GDT_READABLE_FLAG;
    gdt[GDT_USER_CS].flagsAndLimit2 = 0xF | GDT_PAGE_GRANURALITY | GDT_PROTECTED_MODE_FLAG;
    gdt[GDT_USER_DS].limit1 = 0xFFFF;
    gdt[GDT_USER_DS].accessByte = GDT_PRESENT_FLAG | GDT_PRIVILEGE_LEVEL_3 | GDT_DATA_FLAG | GDT_WRITABLE_FLAG;
    gdt[GDT_USER_DS].flagsAndLimit2 = 0xF | GDT_PAGE_GRANURALITY | GDT_PROTECTED_MODE_FLAG;

    gdtr.size = (sizeof(gdt)) - 1;
    gdtr.offset = (uintptr_t)gdt;

    GdtApply();
}

void GdtApply(void)
{
    ASM("lgdt %0" : : "m" (gdtr));
}

STATUS GdtAddCpu(uint16_t cpu)
{
    if(cpu >= MAX_CPU_COUNT)
        return MM_GDT_ENTRY_LIMIT_EXCEEDED;

    tss[cpu].ss0 = GDT_OFFSET(GDT_KERNEL_DS);
    tss[cpu].esp0 = 0;
    tss[cpu].iopb = sizeof(struct TssEntry);

    gdt[GDT_TSS(cpu)].limit1 = sizeof(struct TssEntry) - 1;
    gdt[GDT_TSS(cpu)].base1 = (uintptr_t)(&tss[cpu]) & (uintptr_t)0xFFFF;
    gdt[GDT_TSS(cpu)].base2 = ((uintptr_t)(&tss[cpu]) & (uintptr_t)0xFF0000) >> 16;
    gdt[GDT_TSS(cpu)].base3 = ((uintptr_t)(&tss[cpu]) & (uintptr_t)0xFF000000) >> 24;
    gdt[GDT_TSS(cpu)].flagsAndLimit2 = 0;
    gdt[GDT_TSS(cpu)].accessByte = GDT_PRESENT_FLAG | GDT_32BIT_TSS_FLAG;

    return OK;
}

void GdtLoadTss(uint16_t cpu)
{
    //load GDT descriptor with TSS to task register
    ASM("ltr %0" : : "r" GDT_OFFSET(GDT_TSS(cpu)));
}

void GdtUpdateTss(uintptr_t esp0)
{
    uint16_t register t;
    //get GDT descriptor with TSS from task register
    ASM("str %0" : "=r" (t) :);
    //convert GDT descriptor offset to CPU number and update kernel stack pointer
    tss[GDT_CPU(GDT_ENTRY(t))].esp0 = esp0;
}