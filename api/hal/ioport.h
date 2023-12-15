//This header file is generated automatically
#ifndef EXPORTED___API__HAL_IOPORT_H_
#define EXPORTED___API__HAL_IOPORT_H_

#ifdef __cplusplus
extern "C" 
{
#endif

#include <stdint.h>
#include "defines.h"
/**
 * @brief Read byte from IO port
 * @param port Port number
 * @return Data read
*/
extern uint8_t HalIoPortReadByte(uint16_t port);

/**
 * @brief Write byte to IO port
 * @param port Port number
 * @param d Data to write
*/
extern void HalIoPortWriteByte(uint16_t port, uint8_t d);

/**
 * @brief Read word from IO port
 * @param port Port number
 * @return Data read
*/
extern uint16_t HalIoPortReadWord(uint16_t port);

/**
 * @brief Write word to IO port
 * @param port Port number
 * @param d Data to write
*/
extern void HalIoPortWriteWord(uint16_t port, uint16_t d);

/**
 * @brief Read double word from IO port
 * @param port Port number
 * @return Data read
*/
extern uint32_t HalIoPortReadDWord(uint16_t port);

/**
 * @brief Write double word to IO port
 * @param port Port number
 * @param d Data to write
*/
extern void HalIoPortWriteDWord(uint16_t port, uint32_t d);


#ifdef __cplusplus
}
#endif

#endif