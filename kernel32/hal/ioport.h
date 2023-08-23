#ifndef KERNEL_IOPORT_H_
#define KERNEL_IOPORT_H_

#include <stdint.h>
#include "defines.h"

/**
 * @defgroup ioPort IO port manipulation routines
 * @ingroup hal
 * @{
*/

EXPORT
/**
 * @brief Read byte from IO port
 * @param port Port number
 * @return Data read
*/
EXTERN uint8_t PortIoReadByte(uint16_t port);

EXPORT
/**
 * @brief Write byte to IO port
 * @param port Port number
 * @param d Data to write
*/
EXTERN void PortIoWriteByte(uint16_t port, uint8_t d);

EXPORT
/**
 * @brief Read word from IO port
 * @param port Port number
 * @return Data read
*/
EXTERN uint16_t PortIoReadWord(uint16_t port);

EXPORT
/**
 * @brief Write word to IO port
 * @param port Port number
 * @param d Data to write
*/
EXTERN void PortIoWriteWord(uint16_t port, uint16_t d);

EXPORT
/**
 * @brief Read double word from IO port
 * @param port Port number
 * @return Data read
*/
EXTERN uint32_t PortIoReadDWord(uint16_t port);

EXPORT
/**
 * @brief Write double word to IO port
 * @param port Port number
 * @param d Data to write
*/
EXTERN void PortIoWriteDWord(uint16_t port, uint32_t d);


/**
 * @}
*/


#endif