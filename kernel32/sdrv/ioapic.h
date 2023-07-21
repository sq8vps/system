#ifndef KERNEL_IOAPIC_H_
#define KERNEL_IOAPIC_H_

#include <stdint.h>
#include "defines.h"
#include "hal/interrupt.h"

struct ApicIoEntry
{
   uint8_t id;
   uint32_t address;
};

extern struct ApicIoEntry ApicIoEntryTable[MAX_IOAPIC_COUNT];
extern uint8_t ApicIoEntryCount;


/**
 * @brief Initialize I/O APIC
 * @return Status code
*/
STATUS ApicIoInit(void);

/**
 * @brief Register I/O APIC IRQ
 * @param id I/O APIC ID
 * @param input I/O APIC input pin number
 * @param vector Interrupt vector number
 * @param mode Interrupt delivery mode
 * @param polarity Input polarity
 * @param trigger Interrupt trigger
 * @return Status code
*/
STATUS ApicIoRegisterIRQ(uint8_t id, uint8_t input, uint8_t vector, enum HalInterruptMode mode,
                        enum HalInterruptPolarity polarity, enum HalInterruptTrigger trigger);

/**
 * @brief Enable I/O APIC IRQ
 * @param vector Vector (IRQ) number
 * @return Status code
*/
STATUS ApicIoEnableIRQ(uint8_t vector);

/**
 * @brief Disable I/O APIC IRQ
 * @param vector Vector (IRQ) number
 * @return Status code
*/
STATUS ApicIoDisableIRQ(uint8_t vector);

#endif