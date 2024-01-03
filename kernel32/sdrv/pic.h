#ifndef KERNEL_PIC_H_
#define KERNEL_PIC_H_

#include <stdint.h>
#include "defines.h"
#include <stdbool.h>

#define PIC_INPUT_COUNT 16

/**
 * @brief Send End Of Interrupt for given IRQ to PIC
 * @param input IRQ number
 * @return Error code
*/
INTERNAL STATUS PicSendEoi(uint32_t input);

/**
 * @brief Remap (shift) PIC interrupts
 * @param masterOffset Master PIC interrupt shift
 * @param slaveOffset Slave PIC interrupt shift
*/
INTERNAL void PicRemap(uint8_t masterIrqOffset, uint8_t slaveIrqOffset);

/**
 * @brief Enable given IRQ in PIC
 * @param input IRQ number
 * @return Error code
*/
INTERNAL STATUS PicEnableIrq(uint32_t input);

/**
 * @brief Disable given IRQ in PIC
 * @param input IRQ number
 * @return Error code
*/
INTERNAL STATUS PicDisableIrq(uint32_t input);

/**
 * @brief Set PICs IRQ mask directly
 * @param mask New mask to apply (master in LSB, slave in MSB)
*/
INTERNAL void PicSetIrqMask(uint16_t mask);

/**
 * @brief Get ISR register from both PICs
 * @return ISR register (master in LSB, slave in MSB)
*/
INTERNAL uint16_t PicGetIsr(void);

/**
 * @brief Get IRR register from both PICs
 * @return IRR register (master in LSB, slave in MSB)
*/
INTERNAL uint16_t PicGetIrr(void);

/**
 * @brief Check if interrupt is spurious
 * @return True if spurious, false if not
*/
INTERNAL bool PicIsIrqSpurious(void);

/**
 * @brief Reserve PIC input
 * @param input Requested input or HAL_INTERRUPT_INPUT_ANY for any input
 * @return Reserved input or UINT32_MAX on failure
*/
INTERNAL uint32_t PicReserveInput(uint32_t input);

/**
 * @brief Free PIC input
 * @param input PIC input to be released
*/
INTERNAL void PicFreeInput(uint32_t input);

#endif