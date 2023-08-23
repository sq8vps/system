#ifndef KERNEL_PIC_H_
#define KERNEL_PIC_H_

#include <stdint.h>
#include "defines.h"
#include <stdbool.h>
 
/**
 * In general this driver is not used for anything else but to disable PIC.
 * PIC is extremely old and this OS requires at least the Local APIC to run.
*/

/**
 * @brief Send End Of Interrupt for given IRQ to PIC
 * @param irq Interrupt number
 * @return Error code
*/
STATUS PicSendEOI(uint8_t irq);

/**
 * @brief Remap (shift) PIC interrupts
 * @param masterOffset Master PIC interrupt shift
 * @param slaveOffset Slave PIC interrupt shift
*/
void PicRemap(uint8_t masterOffset, uint8_t slaveOffset);

/**
 * @brief Enable given IRQ in PIC
 * @param irq IRQ number
 * @return Error code
*/
STATUS PicEnableIRQ(uint8_t irq);

/**
 * @brief Disable given IRQ in PIC
 * @param irq IRQ number
 * @return Error code
*/
STATUS PicDisableIRQ(uint8_t irq);

/**
 * @brief Set PICs IRQ mask directly
 * @param mask New mask to apply (master in LSB, slave in MSB)
*/
void PicSetIRQMask(uint16_t mask);

/**
 * @brief Get ISR register from both PICs
 * @return ISR register (master in LSB, slave in MSB)
*/
uint16_t PicGetISR(void);

/**
 * @brief Get IRR register from both PICs
 * @return IRR register (master in LSB, slave in MSB)
*/
uint16_t PicGetIRR(void);

/**
 * @brief Check if IRQ7 was spurious
 * @return 1 if spurious, 0 if not
 * @attention When 1 is returned, the interrupt is false and should not be processed
 * @warning Use only in IRQ7 handler
*/
bool PicCheckSpuriousIRQ7(void);

/**
 * @brief Check if IRQ15 was spurious
 * @return 1 if spurious, 0 if not
 * @attention When 1 is returned, the interrupt is false and should not be processed
 * @warning Use only in IRQ15 handler
*/
bool PicCheckSpuriousIRQ15(void);



#endif