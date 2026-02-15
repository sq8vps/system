#ifndef KERNEL_HAL_H_
#define KERNEL_HAL_H_

/**
 * @file hal.h
 * @brief Hardware Abstraction Layer general routines
 * @ingroup hal
*/

#include <stdint.h>
#include "defines.h"
#include <stdbool.h>

/**
 * @addtogroup hal Hardware Abstraction Layer
 * @brief Hardware Abstraction Layer library
 * 
 * This module provides an abstraction layer for the hardware. There is a submodule provided for each supported architecture.
 */

 /**
  * @addtogroup hal_general General HAL routines
  * @ingroup hal
  * @{
  */

DRIVER_API

/**
 * @brief Check if buffer is accessible in user mode
 * @param *buffer Buffer to be validated
 * @param size Buffer size
 * @return True if valid, false if invalid
 */
bool HalValidateUserBuffer(const void *buffer, size_t size);

END_DRIVER_API

/**
 * @brief Get root device ID
 * @kinternal
 * @return Root device ID pointer
 */
INTERNAL char *HalGetRootDeviceId(void);

/**
 * @brief Set root device ID
 * @param *id Root device ID to be set
 * @kinternal
 * @warning This function should be used only by the architecture-specific HAL module
 */
INTERNAL void HalSetRootDeviceId(const char *id);

/**
 * @brief Architecture-specific initialization phase 1 (pre-scheduler)
 * @kinternal
 */
INTERNAL void HalInitPhase1(void);

/**
 * @brief Architecture-specific initialization phase 2 (pre-scheduler)
 * @kinternal
 */
INTERNAL void HalInitPhase2(void);

/**
 * @brief Architecture-specific initialization phase 3 (pre-scheduler)
 * @kinternal
 */
INTERNAL void HalInitPhase3(void);

/**
 * @brief Architecture-specific initialization phase 3 (post-scheduler)
 * @kinternal
 */
INTERNAL void HalInitPhase4(void);

/**
 * @brief Call global constructors
 * @kinternal
 */
INTERNAL void HalCallConstructors(void);

/**
 * @}
*/

#endif