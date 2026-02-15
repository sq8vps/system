/**
 * @file display.h
 * @brief Display DDK and helpers
 * @ingroup ddk
 */

#ifndef DDK_DISPLAY_H_
#define DDK_DISPLAY_H_

#include <stdint.h>
#include "defines.h"

DRIVER_API

/**
 * @addtogroup ddk_display Display requests and helpers
 * @ingroup ddk
 * 
 * @{
 */

/**
 * @brief Device ID for generic frame buffer-compatible color display device
 */
#define DDK_DISPLAY_GENERIC_COLOR_FB_DEVICE_ID "DISPLAY/GENCOLFB"

/**
 * @}
 */

END_DRIVER_API

#endif