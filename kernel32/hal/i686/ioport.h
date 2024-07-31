#ifndef KERNEL_IOPORT_H_
#define KERNEL_IOPORT_H_

/**
 * @file ioport.h
 * @brief IO port manipulation routines
 * 
 * @defgroup ioport IO port manipulation routines
 * @ingroup i686 amd64
*/

#include <stdint.h>
#include "defines.h"

/**
 * @addtogroup ioport
 * @{
*/

EXPORT
/**
 * @brief Read byte from IO port
 * @param port Port number
 * @return Data read
*/
EXTERN uint8_t IoPortReadByte(uint16_t port);

EXPORT
/**
 * @brief Write byte to IO port
 * @param port Port number
 * @param d Data to write
*/
EXTERN void IoPortWriteByte(uint16_t port, uint8_t d);

EXPORT
/**
 * @brief Read word from IO port
 * @param port Port number
 * @return Data read
*/
EXTERN uint16_t IoPortReadWord(uint16_t port);

EXPORT
/**
 * @brief Write word to IO port
 * @param port Port number
 * @param d Data to write
*/
EXTERN void IoPortWriteWord(uint16_t port, uint16_t d);

EXPORT
/**
 * @brief Read double word from IO port
 * @param port Port number
 * @return Data read
*/
EXTERN uint32_t IoPortReadDWord(uint16_t port);

EXPORT
/**
 * @brief Write double word to IO port
 * @param port Port number
 * @param d Data to write
*/
EXTERN void IoPortWriteDWord(uint16_t port, uint32_t d);

// EXPORT
// /**
//  * @brief Assign a range of consecutive I/O ports
//  * @param count Number of I/O ports
//  * @return First I/O port or 0 on failure
// */
// EXTERN uint16_t HalIoPortAssign(uint16_t count);

// EXPORT
// /**
//  * @brief Free a range of consecutive I/O ports
//  * @param first First I/O port
//  * @param count Number of I/O ports 
// */
// EXTERN void HalIoPortFree(uint16_t first, uint16_t count);

// /**
//  * @brief Initialize I/O port HAL module
// */
// INTERNAL void HalIoPortInit(void);

/**
 * @}
*/


#endif