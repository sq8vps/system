#ifndef KERNEL_GDT_H_
#define KERNEL_GDT_H_

/**
 * @file gdt.h
 * @brief Global Descriptor Table library
 * 
 * Provides mainpulation routines for GDTs.
 * Prepared generally only for flat GDTs.
 * GDTs should not be used for segmentation in the OS.
 * @ingroup mm
*/

#include <stdint.h>
#include "defines.h"

/**
 * @defgroup gdt Global Descriptor Table manipulation routines
 * @ingroup mm
 * @{
*/

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

/**
 * @brief Apply (enable) filled GDT
*/
void MmGdtApply(void);

/**
 * @brief Clear GDT
*/
void MmGdtClear(void);

/**
 * @brief Initialize GDT
*/
void MmGdtInit(void);

/**
 * @brief Prepare and apply flat GDT
*/
void MmGdtApplyFlat(void);

/**
 * @brief Add and load GDT entry for CPU
 * @param cpu CPU index
 * @param *entry GDT entry to add
 * @return Error code
*/
STATUS MmGdtAddCPUEntry(uint16_t cpu, struct GdtEntry *entry);

/**
 * @brief Get privileged code GDT entry offset
 * @return Offset in bytes
*/
uint32_t MmGdtGetFlatPrivilegedCodeOffset(void);

/**
 * @brief Get unprivileged code GDT entry offset
 * @return Offset in bytes
*/
uint32_t MmGdtGetFlatUnprivilegedCodeOffset(void);

/**
 * @brief Get privilieged data GDT entry offset
 * @return Offset in bytes
*/
uint32_t MmGdtGetFlatPrivilegedDataOffset(void);

/**
 * @brief Get unprivilieged data GDT entry offset
 * @return Offset in bytes
*/
uint32_t MmGdtGetFlatUnprivilegedDataOffset(void);

/**
 * @}
*/

#endif