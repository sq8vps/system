#ifndef KERNEL_IOAPIC_H_
#define KERNEL_IOAPIC_H_

#include <stdint.h>
#include "defines.h"
#include "hal/interrupt.h"

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
INTERNAL STATUS ApicIoRegisterIRQ(uint32_t input, uint8_t vector, enum HalInterruptMode mode,
                        enum HalInterruptPolarity polarity, enum HalInterruptTrigger trigger);

/**
 * @brief Unregister I/O APIC IRQ
 * @param input Global interrupt source number
 * @return Status code
*/
INTERNAL STATUS ApicIoUnregisterIRQ(uint32_t input);

/**
 * @brief Enable I/O APIC IRQ
 * @param vector Vector (IRQ) number
 * @return Status code
*/
INTERNAL STATUS ApicIoEnableIRQ(uint8_t vector);

/**
 * @brief Disable I/O APIC IRQ
 * @param vector Vector (IRQ) number
 * @return Status code
*/
INTERNAL STATUS ApicIoDisableIRQ(uint8_t vector);

#endif