#ifndef I686_IRQ_H_
#define I686_IRQ_H_

#include "defines.h"
#include <stdbool.h>

/**
 * @brief Set dual-PIC presence state
 * @param state Dual-PIC presence state
 */
INTERNAL void I686SetDualPicPresence(bool state);

EXPORT
/**
 * @brief Get I/O APIC usage state
 * @return True if I/O APIC is used, false otherwise
 */
EXTERN bool I686IsIoApicUsed(void);


/**
 * @brief Set default (1:1) ISA IRQ mapping
 */
INTERNAL void I686SetDefaultIsaRemap(void);

#endif