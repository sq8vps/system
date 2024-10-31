#ifndef I686_IRQ_H_
#define I686_IRQ_H_

#include "defines.h"
#include <stdbool.h>


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
 * @return Status code
*/
INTERNAL STATUS I686AddIsaRemapEntry(uint8_t isaIrq, uint32_t gsi);

#endif