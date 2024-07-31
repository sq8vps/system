#ifndef KERNEL_DRIVER_HAL
#define KERNEL_DRIVER_HAL

/**
 * @file hal.h
 * @brief Hardware Abstraction Layer driver general routines
 * 
 * Provides general routines for system HAL.
 * 
 * @defgroup hal Hardware Abstraction Layer driver
*/

#include <stdint.h>
#include "defines.h"
#include <stdbool.h>

/**
 * @addtogroup hal
 * @{
*/

/**
 * @brief Initialize architecture-specific basic hardware
 * @return Status code 
 */
INTERNAL STATUS HalInitializeHardware(void);

/**
 * @brief Initialize fundamentals of a root device
 * @return Status code 
 */
INTERNAL STATUS HalInitializeRoot(void);

/**
 * @brief Initialize hardware abstraction layer
 * 
 * Initializes whole Hardware Abstraction Layer. Reads system tables and sets up all core peripherals.
 * 
 * @return Status code
*/
INTERNAL STATUS HalInit(void);

/**
 * @brief Get root device ID
 * @return Root device ID pointer
 */
INTERNAL char *HalGetRootDeviceId(void);

/**
 * @brief Set root device ID
 * @param *id Root device ID to be set
 * @warning This function should be used only by the architecture-specific HAL module
 */
INTERNAL void HalSetRootDeviceId(const char *id);



/**
 * @}
*/

#endif