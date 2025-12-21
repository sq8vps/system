#ifndef HAL_I686_UART_H
#define HAL_I686_UART_H

#include "defines.h"

EXPORT_API

/**
 * @brief Deinitialize built-in 16550-compatible UART driver
 */
void I686DeinitIsaUart(void);

END_EXPORT_API

#endif