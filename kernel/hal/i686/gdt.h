/**
 * @file gdt.h
 * @brief Global Descriptor Table and Task State Segment support
 * @ingroup i686
 * @kinternal
 * @note This
 */

#ifndef I686_GDT_H_
#define I686_GDT_H_


#include <stdint.h>
#include "defines.h"

struct KeTaskControlBlock;

/**
 * @addtogroup i686_gdt GDT and TSS support
 * @ingroup i686
 * @kinternal
 * 
 * This module provides support for GDT, TSS, and TLS (low-level). 
 * For GDT, only flat memory model is supported.
 * @{
*/

/**
 * @brief Kernel code segment descriptor index
 */
#define GDT_KERNEL_CS 1

/**
 * @brief Kernel data segment descriptor index
 */
#define GDT_KERNEL_DS 2

/**
 * @brief User code segment descriptor index
 */
#define GDT_USER_CS 3

/**
 * @brief User data segment descriptor index
 */
#define GDT_USER_DS 4

/**
 * @brief TSS descriptor index for given CPU
 * @param cpu CPU number
 */
#define GDT_TSS(cpu) (5 + (cpu))

/**
 * @brief TLS descriptor index for given CPU
 * @param cpu CPU number
 */
#define GDT_TLS(cpu) (5 + MAX_CPU_COUNT + (cpu))

/**
 * @brief Get GDT offset from entry number
 */
#define GDT_OFFSET(entry) (8 * (entry))

/**
 * @brief Get user mode selector from GDT entry number
 */
#define USER_SELECTOR(gdtEntry) (GDT_OFFSET((gdtEntry)) | 0x3)

/**
 * @brief Get GDT entry number from offset
 */
#define GDT_ENTRY(offset) ((offset) / 8)

/**
 * @brief Get CPU number from GDT entry number
 */
#define GDT_CPU(entry) ((entry) - 5)

/**
 * @brief Initialize Global Descriptor Table and apply to current CPU
 */
INTERNAL void GdtInit(void);

/**
 * @brief Apply GDT to current CPU
 * @param cpu CPU number
 * @attention GDT must be initialized first with \a GdtInit()
 */
INTERNAL void GdtApply(uint32_t cpu);

/**
 * @brief Create TSS for CPU and add to GDT
 * @param cpu CPU number
 * @return Status code
 */
INTERNAL STATUS GdtAddCpu(uint32_t cpu);

/**
 * @brief Load TSS to Task Register
 * @param cpu CPU number
 * @warning This function must be called by the target CPU
 */
INTERNAL void GdtLoadTss(uint32_t cpu);

/**
 * @brief Update kernel stack pointer for current CPU
 * @param esp0 New kernel stack pointer
 */
FASTCALL
INTERNAL void GdtUpdateTss(uintptr_t esp0);

/**
 * @brief Update GDT entry for TLS and load GS
 * @param cpu CPU number
 * @param *tcb Task Control Block
 * @warning This function must be called by the target CPU
 */
FASTCALL
void GdtRestoreTls(uint32_t cpu, const struct KeTaskControlBlock *tcb);

/**
 * @}
*/

#endif