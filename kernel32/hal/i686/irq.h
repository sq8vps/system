/**
 * @file irq.h
 * @brief IRQ controller (PIC/IOAPIC) support
 * @ingroup i686
 * @note This module implements the universal HAL interface and most of its function are available using kernel API.
 */

#ifndef I686_IRQ_H_
#define I686_IRQ_H_

#include "defines.h"
#include <stdbool.h>

/**
 * @addtogroup i686_irq IRQ controller support
 * @brief IRQ controller support
 * @ingroup i686
 * 
 * This module provides IRQ controller support and abstracts PIC and I/O APIC.
 * It also acts as a legacy ISA IRQ resolver.
 * @{
 */

EXPORT_API

/**
 * @brief Get I/O APIC usage state
 * @return True if I/O APIC is used, false otherwise
 */
bool I686IsIoApicUsed(void);

/**
 * @brief Resolve legacy ISA IRQ to global interrupt mapping
 * @param irq ISA IRQ from device
 * @return Resolved IRQ after remapping (if applicable)
*/
uint32_t I686ResolveIsaIrqMapping(uint32_t irq);

/**
 * @brief Get legacy ISA IRQ parameters
 * @param irq ISA IRQ from device
 * @return Resolved ISA IRQ parameters
*/
struct HalInterruptParams I686ResolveIsaIrqParams(uint32_t irq);

END_EXPORT_API

/**
 * @brief Set default (1:1) ISA IRQ mapping
 */
INTERNAL void I686SetDefaultIsaRemap(void);

/**
 * @brief Set dual-PIC presence state
 * @param state Dual-PIC presence state
 */
INTERNAL void I686SetDualPicPresence(bool state);

/**
 * @brief Initialize I/O APIC or PIC
 * @return Status code
 */
INTERNAL STATUS I686InitInterruptController(void);

/**
 * @brief Add ISA remap entry when I/O APIC is used
 * @param isaIrq Original ISA IRQ
 * @param gsi Global System Interrupt (Global IRQ number)
 * @param params Interrupt parameters
 * @return Status code
*/
INTERNAL STATUS I686AddIsaRemapEntry(uint8_t isaIrq, uint32_t gsi, struct HalInterruptParams params);

/**
 * @}
 */

#endif