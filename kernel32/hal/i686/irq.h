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

#endif