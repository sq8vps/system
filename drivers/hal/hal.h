#ifndef KERNEL_DRIVER_HAL
#define KERNEL_DRIVER_HAL

#include <stdint.h>

/**
 * @file hal.h
 * @brief Hardware Abstraction Layer driver
 * 
 * Provides hardware manipulation routines for other drivers
 * 
 * @defgroup hal Hardware Abstraction Layer driver
*/

/**
 * @defgroup ioPort IO port manipulation routines
 * @ingroup hal
 * @{
*/

/**
 * @brief Read byte from IO port
 * @param port Port number
 * @return Data read
*/
extern uint8_t Hal_IOPortReadByte(uint16_t port);

/**
 * @brief Write byte to IO port
 * @param port Port number
 * @param d Data to write
*/
extern void Hal_IOPortWriteByte(uint16_t port, uint8_t d);

/**
 * @brief Read word from IO port
 * @param port Port number
 * @return Data read
*/
extern uint16_t Hal_IOPortReadWord(uint16_t port);

/**
 * @brief Write word to IO port
 * @param port Port number
 * @param d Data to write
*/
extern void Hal_IOPortWriteWord(uint16_t port, uint16_t d);

/**
 * @brief Read double word from IO port
 * @param port Port number
 * @return Data read
*/
extern uint32_t Hal_IOPortReadDWord(uint16_t port);

/**
 * @brief Write double word to IO port
 * @param port Port number
 * @param d Data to write
*/
extern void Hal_IOPortWriteDWord(uint16_t port, uint32_t d);

/**
 * @}
*/

#endif