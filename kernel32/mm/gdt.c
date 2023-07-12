#include "gdt.h"
#include "../common.h"

#define GDT_MAX_ENTRIES (4 + MAX_CPU_COUNT)

#define GDT_KERNEL_CODE_ENTRY 1
#define GDT_USER_CODE_ENTRY 3
#define GDT_CPU0_ENTRY 5

static struct GdtEntry gdt[GDT_MAX_ENTRIES + 1]; //table of GDTs

/**
 * @brief A GDTR structure
*/
struct GdtRegister
{
    uint16_t size;
    uintptr_t offset;
} __attribute__ ((packed)) gdtr;

void MmGdtApply(void)
{
    gdtr.size = (sizeof(gdt)) - 1;
    gdtr.offset = (uintptr_t)gdt;

    asm volatile("lgdt %0" : : "m" (gdtr));
}

void MmGdtClear(void)
{
    Cm_memset(gdt, 0, sizeof(gdt)); //clear all entries
}

void MmGdtInit(void)
{
    MmGdtClear();
}

void MmGdtApplyFlat(void)
{
    gdt[GDT_KERNEL_CODE_ENTRY].limit1 = 0xFFFF;
    gdt[GDT_KERNEL_CODE_ENTRY].accessByte = GDT_PRESENT_FLAG | GDT_PRIVILEGE_LEVEL_0 | GDT_CODE_FLAG | GDT_EXECUTABLE_FLAG | GDT_READABLE_FLAG;
    gdt[GDT_KERNEL_CODE_ENTRY].flagsAndLimit2 = 0xF | GDT_PAGE_GRANURALITY | GDT_PROTECTED_MODE_FLAG;
    gdt[GDT_KERNEL_CODE_ENTRY + 1].limit1 = 0xFFFF;
    gdt[GDT_KERNEL_CODE_ENTRY + 1].accessByte = GDT_PRESENT_FLAG | GDT_PRIVILEGE_LEVEL_0 | GDT_DATA_FLAG | GDT_WRITABLE_FLAG;
    gdt[GDT_KERNEL_CODE_ENTRY + 1].flagsAndLimit2 = 0xF | GDT_PAGE_GRANURALITY | GDT_PROTECTED_MODE_FLAG;
    gdt[GDT_USER_CODE_ENTRY].limit1 = 0xFFFF;
    gdt[GDT_USER_CODE_ENTRY].accessByte = GDT_PRESENT_FLAG | GDT_PRIVILEGE_LEVEL_3 | GDT_CODE_FLAG | GDT_EXECUTABLE_FLAG | GDT_READABLE_FLAG;
    gdt[GDT_USER_CODE_ENTRY].flagsAndLimit2 = 0xF | GDT_PAGE_GRANURALITY | GDT_PROTECTED_MODE_FLAG;
    gdt[GDT_USER_CODE_ENTRY + 1].limit1 = 0xFFFF;
    gdt[GDT_USER_CODE_ENTRY + 1].accessByte = GDT_PRESENT_FLAG | GDT_PRIVILEGE_LEVEL_3 | GDT_DATA_FLAG | GDT_WRITABLE_FLAG;
    gdt[GDT_USER_CODE_ENTRY + 1].flagsAndLimit2 = 0xF | GDT_PAGE_GRANURALITY | GDT_PROTECTED_MODE_FLAG;

    MmGdtApply();
}

STATUS MmGdtAddCPUEntry(uint16_t cpu, struct GdtEntry *entry)
{
    if(cpu >= MAX_CPU_COUNT)
        return MM_GDT_ENTRY_LIMIT_EXCEEDED;

    gdt[GDT_CPU0_ENTRY + cpu] = *entry;
}

uint32_t MmGdtGetFlatPrivilegedCodeOffset(void)
{
    return GDT_KERNEL_CODE_ENTRY * sizeof(struct GdtEntry);
}

uint32_t MmGdtGetFlatPrivilegedDataOffset(void)
{
    return (GDT_KERNEL_CODE_ENTRY + 1) * sizeof(struct GdtEntry);
}

uint32_t MmGdtGetFlatUnprivilegedCodeOffset(void)
{
    return GDT_USER_CODE_ENTRY * sizeof(struct GdtEntry);
}

uint32_t MmGdtGetFlatUnprivilegedDataOffset(void)
{
    return (GDT_USER_CODE_ENTRY + 1) * sizeof(struct GdtEntry);
}
