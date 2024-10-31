#ifndef KERNEL_HAL_H_
#define KERNEL_HAL_H_

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

EXPORT_API

/**
 * @brief Check if buffer is accessible in user mode
 * @param *buffer Buffer to be validated
 * @param size Buffer size
 * @return True if valid, false if invalid
 */
bool HalValidateUserBuffer(const void *buffer, uintptr_t size);

END_EXPORT_API

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
 * @brief Architecture-specific initialization phase 1
 */
INTERNAL void HalInitPhase1(void);

/**
 * @brief Architecture-specific initialization phase 2
 */
INTERNAL void HalInitPhase2(void);

/**
 * @brief Architecture-specific initialization phase 3
 */
INTERNAL void HalInitPhase3(void);

/**
 * @brief Call global constructor for C++ support
 */
INTERNAL void HalCallConstructors(void);


/**
 * @}
*/

#endif