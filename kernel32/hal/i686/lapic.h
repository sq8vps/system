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
 * @brief APIC IPI destination shorthand modes
 */
enum ApicIpiDestination
{
    APIC_IPI_DESTINATION_NORMAL = 0,
    APIC_IPI_DESTINATION_SELF = 1,
    APIC_IPI_DESTINATION_ALL = 2,
    APIC_IPI_DESTINATION_ALL_BUT_SELF = 3,
};

/**
 * @brief Send inter-processor interrupt
 * @param shorthand Destination mode
 * @param destination Destination LAPIC ID
 * @param mode IPI mode
 * @param vector Vector number
 * @param assert True to assert, false to deassert
 */
INTERNAL void ApicSendIpi(enum ApicIpiDestination shorthand, uint8_t destination, enum ApicIpiMode mode, uint8_t vector, bool assert);

/**
 * @brief Wait for IPI delivery up to a given time
 * @param timeLimit Time limit in nanoseconds
 * @return \a OK if IPI delivered, \a TIMEOUT if delivery timed out
 */
INTERNAL STATUS ApicWaitForIpiDelivery(uint64_t timeLimit);

/**
 * @brief Send End Of Interrupt using APIC
 * @return Status code
*/
INTERNAL STATUS ApicSendEoi(void);


/**
 * @brief Initialize APIC on Application Processor
 * @return Status code
 * @warning Bootstrap processor's APIC must be initialized first
*/
INTERNAL STATUS ApicInitAp(void);

/**
 * @brief Initialize APIC on Bootstrap Processor
 * @param address Local APIC physical address
 * @return Status code
*/
INTERNAL STATUS ApicInitBsp(void);

/**
 * @brief Initialize APIC module - map LAPIC memory
 * @param address LAPIC physical address
 * @return Status code
 */
INTERNAL STATUS ApicInit(uintptr_t address);

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

/**
 * @brief Get current CPU Local APIC ID
 * @return LAPIC ID
 */
INTERNAL uint8_t ApicGetCurrentId(void);

/**
 * @brief Apply real time fix to all APIC counters
 * @param realTime Real time in ns
 */
INTERNAL void ApicSetRealTime(uint64_t realTime);

#endif