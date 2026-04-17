/**
 * @file lapic.h
 * @brief Local APIC support module
 * @ingroup i686
 * @note This driver implements the universal HAL interface and most of its function are available using kernel API.
 */

#ifndef KERNEL_LAPIC_H_
#define KERNEL_LAPIC_H_

#include <stdint.h>
#include "defines.h"
#include <stdbool.h>

/**
 * @addtogroup i686_lapic Local APIC support module
 * @brief Local APIC support module
 * @kinternal
 * 
 * This module provides support for LAPIC. This includes handling processor priorities, system timer, IPIs (low-level),
 * and sending EOIs.
 * While the LAPIC timer can be used as a time source, kernel mode drivers should use HAL to obtain timestamps.
 * @{
 */

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
 * @brief Update the system timer on interrupt
 */
INTERNAL void ApicUpdateSystemTimerOnInterrupt(void);

/**
 * @brief Start system timer
 * @param time Time to next IRQ in nanoseconds
*/
INTERNAL void ApicStartSystemTimer(uint64_t time);

/**
 * @brief Synchronize APIC timers across all CPUs
 */
INTERNAL void ApicSynchronizeTimers(void);

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
 * @}
 */

#endif