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
 * @brief Initialize hardware abstraction layer
 * @return Status code
 * 
 * Initializes whole Hardware Abstraction Layer. Reads system tables and sets up all core peripherals.
*/
INTERNAL STATUS HalInit(void);

/**
 * @}
*/

#endif