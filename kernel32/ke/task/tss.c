#include "tss.h"
#include "mm/gdt.h"

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

static struct TssEntry tssTable[MAX_CPU_COUNT];

STATUS KePrepareTSS(uint16_t cpu)
{
    if(MAX_CPU_COUNT <= cpu)
        return KE_TSS_ENTRY_LIMIT_EXCEEDED;
    
    tssTable[cpu].ss0 = MmGdtGetFlatPrivilegedDataOffset();
    tssTable[cpu].esp0 = 0;
    tssTable[cpu].iopb = sizeof(*tssTable);

    struct GdtEntry gdt;
    gdt.limit1 = sizeof(*tssTable) - 1;
    gdt.base1 = (uintptr_t)(&tssTable[cpu]) & (uintptr_t)0xFFFF;
    gdt.base2 = ((uintptr_t)(&tssTable[cpu]) & (uintptr_t)0xFF0000) >> 16;
    gdt.base3 = ((uintptr_t)(&tssTable[cpu]) & (uintptr_t)0xFF000000) >> 24;
    gdt.flagsAndLimit2 = GDT_PROTECTED_MODE_FLAG;
    gdt.accessByte = GDT_PRESENT_FLAG | GDT_32BIT_TSS_FLAG;

    return MmGdtAddCPUEntry(cpu, &gdt);
}

void KeUpdateTSS(uint16_t cpu, uintptr_t esp0)
{
    tssTable[cpu].esp0 = esp0;
}