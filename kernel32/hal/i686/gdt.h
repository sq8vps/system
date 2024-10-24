#ifndef I686_GDT_H_
#define I686_GDT_H_

/**
 * @file gdt.h
 * @brief Global Descriptor Table and Task State Segment module
 * 
 * Provides a basic handling of GDT and TSS.
 * Only flat memory model is supported.
 * 
 * @defgroup gdt GDT and TSS module
 * @ingroup i686
*/

#include <stdint.h>
#include "defines.h"

/**
 * @addtogroup gdt
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
 * @brief Get GDT offset from entry number
 */
#define GDT_OFFSET(entry) (8 * (entry))

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
 * @attention GDT must be initialized first with \a GdtInit()
 */
INTERNAL void GdtApply(void);

/**
 * @brief Create TSS for CPU and add to GDT
 */
INTERNAL STATUS GdtAddCpu(uint16_t cpu);

/**
 * @brief Load TSS to Task Register
 * @param cpu CPU number
 * @warning This function must be called by the target CPU
 */
INTERNAL void GdtLoadTss(uint16_t cpu);

/**
 * @brief Update kernel stack pointer for current CPU
 * @param esp0 New kernel stack pointer
 */
__attribute__((fastcall))
INTERNAL void GdtUpdateTss(uintptr_t esp0);

/**
 * @}
*/

#endif