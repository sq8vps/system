#ifndef KERNEL_LAPIC_H_
#define KERNEL_LAPIC_H_

#include <stdint.h>
#include "defines.h"



/**
 * @brief Send End Of Interrupt using APIC
 * @return Status code
*/
STATUS ApicSendEOI(void);

/**
 * @brief Set NMI input
 * @param lint Local Interrupt port number (0 or 1)
 * @param mpflags MP-compliant interrupt flags
 * @return Status code
*/
// STATUS ApicSetNMI(uint8_t lint, uint16_t mpflags);

/**
 * @brief Initialize APIC on Application Processor
 * @return Status code
 * @brief Bootstrap processor's APIC must be initialized first
*/
STATUS ApicInitAP(void);

/**
 * @brief Initialize APIC on Bootstrap Processor
 * @param address Local APIC physical address
 * @return Status code
*/
STATUS ApicInitBSP(uintptr_t address);

/**
 * @brief Enable local or I/O APIC IRQ
 * @param vector Vector (IRQ) number
 * @return Status code
*/
STATUS ApicEnableIRQ(uint8_t vector);

/**
 * @brief Disable local or I/O APIC IRQ
 * @param vector Vector (IRQ) number
 * @return Status code
*/
STATUS ApicDisableIRQ(uint8_t vector);

/**
 * @brief Initialize system timer
 * @param interval Timer interval in milliseconds
 * @param vector Interrupt vector number
 * @attention Interrupt must be enabled with HalEnableIRQ() or ApicEnableIRQ()
*/
STATUS ApicInitSystemTimer(uint32_t interval, uint8_t vector);


/**
 * @brief Restart system counter (start couting again)
*/
void ApicRestartSystemTimer(void);

#endif