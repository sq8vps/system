/**
 * @file debug.h
 * @brief Hardware kernel debugger
 * @ingroup hal
 */

#ifndef KERNEL_HAL_DEBUG_H_
#define KERNEL_HAL_DEBUG_H_

#include "defines.h"

/**
 * @addtogroup hal_debug Hardware kernel debugger
 * @brief Hardware kernel debugger abstraction layer
 * @kinternal
 * @ingroup hal
 * @{
 */

/**
 * @brief Initialize low-level debugging port
 * @return Status code
 */
INTERNAL STATUS HalDebugPortInit(void);

/**
 * @brief Put character on debugging port
 * @param c Character to put
 * @return Status code
 */
INTERNAL STATUS HalDebugPutChar(char c);

/**
 * @brief Put null-terminated string on debugging port
 * @param *str String to put
 * @return Status code
 */
INTERNAL STATUS HalDebugPutString(const char *str);

/**
 * @brief Put size-limited null-terminated string on debugging port
 * @param *str String to put
 * @param n Size limit
 * @return Status code
 */
INTERNAL STATUS HalDebugPutStringN(const char *str, size_t n);

/**
 * @brief Clear debugging port receive buffer
 * @return Status code
 */
INTERNAL STATUS HalDebugClearRxBuffer(void);

/**
 * @brief Clear debugging port transmit buffer
 * @return Status code
 */
INTERNAL STATUS HalDebugClearTxBuffer(void);

/**
 * @brief Check if any received data is available on debugging port
 * @return True if data available, false otherwise
 */
INTERNAL bool HalDebugIsDataAvailable(void);


/**
 * @}
 */

#endif