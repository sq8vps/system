/**
 * @file color.h
 * @brief Kernel color definitions
 * @ingroup rtl
 */

#ifndef RTL_COLOR_H_
#define RTL_COLOR_H_

#include <stdint.h>
#include "defines.h"

/**
 * @addtogroup rtl
 * @{
 */

EXPORT_API

/**
 * @brief 24-bit color representation structure
 */
typedef struct 
{
    uint8_t r; /**< Red part */
    uint8_t g; /**< Green part */
    uint8_t b; /**< Blue part */
} RtlRGB;

#define RTL_RGB_BLACK (RtlRGB){.r = 0, .g = 0, .b = 0} /**< Predefined black color */
#define RTL_RGB_WHITE (RtlRGB){.r = 255, .g = 255, .b = 255} /**< Predefined white color */
#define RTL_RGB_RED (RtlRGB){.r = 255, .g = 0, .b = 0} /**< Predefined red color */
#define RTL_RGB_GREEN (RtlRGB){.r = 0, .g = 255, .b = 0} /**< Predefined green color */
#define RTL_RGB_BLUE (RtlRGB){.r = 0, .g = 0, .b = 255} /**< Predefined blue color */

END_EXPORT_API

/**
 * @}
 */

#endif