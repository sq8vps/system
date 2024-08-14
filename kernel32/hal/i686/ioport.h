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

EXPORT_API


/**
 * @brief Read byte from IO port
 * @param port Port number
 * @return Data read
*/
uint8_t IoPortReadByte(uint16_t port);


/**
 * @brief Write byte to IO port
 * @param port Port number
 * @param d Data to write
*/
void IoPortWriteByte(uint16_t port, uint8_t d);


/**
 * @brief Read word from IO port
 * @param port Port number
 * @return Data read
*/
uint16_t IoPortReadWord(uint16_t port);


/**
 * @brief Write word to IO port
 * @param port Port number
 * @param d Data to write
*/
void IoPortWriteWord(uint16_t port, uint16_t d);


/**
 * @brief Read double word from IO port
 * @param port Port number
 * @return Data read
*/
uint32_t IoPortReadDWord(uint16_t port);


/**
 * @brief Write double word to IO port
 * @param port Port number
 * @param d Data to write
*/
void IoPortWriteDWord(uint16_t port, uint32_t d);

END_EXPORT_API


// 
// /**
//  * @brief Assign a range of consecutive I/O ports
//  * @param count Number of I/O ports
//  * @return First I/O port or 0 on failure
// */
// uint16_t HalIoPortAssign(uint16_t count);

// 
// /**
//  * @brief Free a range of consecutive I/O ports
//  * @param first First I/O port
//  * @param count Number of I/O ports 
// */
// void HalIoPortFree(uint16_t first, uint16_t count);

// /**
//  * @brief Initialize I/O port HAL module
// */
// INTERNAL void HalIoPortInit(void);

/**
 * @}
*/


#endif