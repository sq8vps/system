#ifndef KERNEL_LAPIC_H_
#define KERNEL_LAPIC_H_

#include <stdint.h>
#include "defines.h"
#include <stdbool.h>

/**
 * @brief APIC IPI modes
 */
enum ApicIpiMode
{
    APIC_IPI_FIXED = 0,
    APIC_IPI_LOWEST_PRIORITY = 1,
    APIC_IPI_LEVEL_SMI = 2,
    APIC_IPI_LEVEL_NMI = 4,
    APIC_IPI_INIT = 5,
    APIC_IPI_START_UP = 6,
};

/**
 * @brief Send inter-processor interrupt
 * @param destination Destination LAPIC ID
 * @param mode IPI mode
 * @param vector Vector number
 * @param assert True to assert, false to deassert
 * @return Status code
 */
STATUS ApicSendIpi(uint8_t destination, enum ApicIpiMode mode, uint8_t vector, bool assert);

void ApicWaitForIpiDelivery(void);

/**
 * @brief Send End Of Interrupt using APIC
 * @return Status code
*/
INTERNAL STATUS ApicSendEoi(void);


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

/**
 * @brief Set current task priority in TPR register
 * @param priority New task priority (0-15)
 * @return Status code
*/
INTERNAL STATUS ApicSetTaskPriority(uint8_t priority);

/**
 * @brief Get current task priority from TPR register
 * @return Current task priority (0-15)
*/
INTERNAL uint8_t ApicGetTaskPriority(void);

/**
 * @brief Get current processor priority from PPR register
 * @return Current processor priority (0-15)
*/
INTERNAL uint8_t ApicGetProcessorPriority(void);

#endif