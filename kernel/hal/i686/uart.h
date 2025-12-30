/**
 * @file uart.h
 * @brief 16550-compatible UART driver
 * @ingroup i686
 * @note This driver implements the universal HAL interface and most of its function are available using kernel API.
 */

#ifndef HAL_I686_UART_H
#define HAL_I686_UART_H

#include "defines.h"

/**
 * @addtogroup i686
 * @{
 */

EXPORT_API

/**
 * @brief Deinitialize built-in 16550-compatible UART driver
 */
void I686DeinitIsaUart(void);

/**
 * @}
 */

END_EXPORT_API

#endif