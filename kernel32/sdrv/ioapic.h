#ifndef KERNEL_IOAPIC_H_
#define KERNEL_IOAPIC_H_

#include <stdint.h>
#include "defines.h"
#include "hal/interrupt.h"

#define IOAPIC_MAX_INPUTS 240

struct ApicIoEntry
{
   uint8_t id;
   uint32_t address;
   uint32_t irqBase;
};

extern struct ApicIoEntry ApicIoEntryTable[MAX_IOAPIC_COUNT];
extern uint8_t ApicIoEntryCount;


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
 * @param vector Vector (IRQ) number
 * @return Status code
*/
INTERNAL STATUS ApicIoEnableIrq(uint8_t vector);

/**
 * @brief Disable I/O APIC IRQ
 * @param vector Vector (IRQ) number
 * @return Status code
*/
INTERNAL STATUS ApicIoDisableIrq(uint8_t vector);

/**
 * @brief Reserve IO APIC input
 * @param input Requested input or 0 for any input
 * @return Reserved input or UINT32_MAX on failure
*/
INTERNAL uint32_t ApicIoReserveInput(uint32_t input);

/**
 * @brief Free IO APIC input
 * @param input IO APIC input to be released
*/
INTERNAL void ApicIoFreeInput(uint32_t input);

#endif