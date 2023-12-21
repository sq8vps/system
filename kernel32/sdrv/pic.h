#ifndef KERNEL_PIC_H_
#define KERNEL_PIC_H_

#include <stdint.h>
#include "defines.h"
#include <stdbool.h>

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
 * @brief Check if interrupt is spurious
 * @return True if spurious, false if not
*/
bool PicIsIrqSpurious(void);


#endif