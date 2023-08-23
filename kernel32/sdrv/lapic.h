#ifndef KERNEL_LAPIC_H_
#define KERNEL_LAPIC_H_

#include <stdint.h>
#include "defines.h"



/**
 * @brief Send End Of Interrupt using APIC
 * @return Status code
*/
INTERNAL STATUS ApicSendEOI(void);

/**
 * @brief Set NMI input
 * @param lint Local Interrupt port number (0 or 1)
 * @param mpflags MP-compliant interrupt flags
 * @return Status code
*/
//INTERNAL STATUS ApicSetNMI(uint8_t lint, uint16_t mpflags);

/**
 * @brief Initialize APIC on Application Processor
 * @return Status code
 * @brief Bootstrap processor's APIC must be initialized first
*/
INTERNAL STATUS ApicInitAP(void);

/**
 * @brief Initialize APIC on Bootstrap Processor
 * @param address Local APIC physical address
 * @return Status code
*/
INTERNAL STATUS ApicInitBSP(uintptr_t address);

/**
 * @brief Enable local or I/O APIC IRQ
 * @param vector Vector (IRQ) number
 * @return Status code
*/
INTERNAL STATUS ApicEnableIRQ(uint8_t vector);

/**
 * @brief Disable local or I/O APIC IRQ
 * @param vector Vector (IRQ) number
 * @return Status code
*/
INTERNAL STATUS ApicDisableIRQ(uint8_t vector);

/**
 * @brief Configure system timer
 * @param vector Interrupt vector number
 * @attention Interrupt must be enabled and timer deadline must be set
*/
INTERNAL STATUS ApicConfigureSystemTimer(uint8_t vector);

/**
 * @brief Start one-shot system timer
 * @param time Time to next IRQ in microseconds
*/
INTERNAL void ApicStartSystemTimer(uint64_t time);

/**
 * @brief Get APIC timestamp in nanoseconds
 * @return Timestamp in nanoseconds
*/
INTERNAL uint64_t ApicGetTimestamp(void);

/**
 * @brief Get APIC timestamp in microseconds
 * @return Timestamp in microseconds
*/
INTERNAL uint64_t ApicGetTimestampMicros(void);

/**
 * @brief Get APIC timestamp in milliseconds
 * @return Timestamp in milliseconds
*/
INTERNAL uint64_t ApicGetTimestampMillis(void);

#endif