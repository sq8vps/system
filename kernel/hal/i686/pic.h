/**
 * @file pic.h
 * @brief 8259 PIC support module
 * @ingroup i686
 */

#ifndef KERNEL_PIC_H_
#define KERNEL_PIC_H_

#include <stdint.h>
#include "defines.h"
#include <stdbool.h>

/**
 * @addtogroup i686_pic 8259 PIC driver
 * @brief 8259 PIC drivers
 * @ingroup i686
 * @kinternal
 * 
 * This module is a 8259 PIC driver. It is only for internal use. Kernel mode drivers should use
 * routines provided by the HAL.
 * @{
 */

 /**
  * @brief Total number of input pins on dual 8259 PIC
  */
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
 * @brief Enable PIC
 */
INTERNAL void PicEnable(void);

/**
 * @brief Disable PIC completely
 * @attention Do not use this function to disable interrupts
 */
INTERNAL void PicDisable(void);

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
 * @param vector Generated interrupt vector
 * @return True if spurious, false if not
*/
INTERNAL bool PicIsIrqSpurious(uint8_t vector);

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

/**
 * @}
 */

#endif