#ifndef KERNEL_IOAPIC_H_
#define KERNEL_IOAPIC_H_

#include <stdint.h>
#include "defines.h"
#include "hal/interrupt.h"

#define IOAPIC_MAX_INPUTS 240

/**
 * @brief Add I/O APIC entry
 * @param id I/O APIC id
 * @param address I/O APIC base address
 * @param irqBase Base (first) IRQ
 */
INTERNAL void ApicIoAddEntry(uint8_t id, uint32_t address, uint32_t irqBase);

/**
 * @brief Initialize I/O APIC
 * @return Status code
*/
INTERNAL STATUS ApicIoInit(void);

/**
 * @brief Get vector number associated with given I/O APIC input
 * @param input Global interrupt source number
 * @return Associated vector number or 0 on failure
*/
INTERNAL uint8_t ApicIoGetAssociatedVector(uint32_t input);

/**
 * @brief Register I/O APIC IRQ
 * @param input Global interrupt source number
 * @param vector Interrupt vector number
 * @param mode Interrupt delivery mode
 * @param polarity Input polarity
 * @param trigger Interrupt trigger
 * @return Status code
*/
INTERNAL STATUS ApicIoRegisterIrq(uint32_t input, uint8_t vector, enum HalInterruptMode mode,
                        enum HalInterruptPolarity polarity, enum HalInterruptTrigger trigger);

/**
 * @brief Unregister I/O APIC IRQ
 * @param input Global interrupt source number
 * @return Status code
*/
INTERNAL STATUS ApicIoUnregisterIrq(uint32_t input);

/**
 * @brief Enable I/O APIC IRQ
 * @param input Input (IRQ) number
 * @return Status code
*/
INTERNAL STATUS ApicIoEnableIrq(uint32_t input);

/**
 * @brief Disable I/O APIC IRQ
 * @param input Input (IRQ) number
 * @return Status code
*/
INTERNAL STATUS ApicIoDisableIrq(uint32_t input);

/**
 * @brief Reserve IO APIC input
 * @param input Requested input or HAL_INTERRUPT_INPUT_ANY for any input
 * @return Reserved input or UINT32_MAX on failure
*/
INTERNAL uint32_t ApicIoReserveInput(uint32_t input);

/**
 * @brief Free IO APIC input
 * @param input IO APIC input to be released
*/
INTERNAL void ApicIoFreeInput(uint32_t input);

#endif