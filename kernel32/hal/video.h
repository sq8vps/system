#ifndef HAL_VIDEO_H_
#define HAL_VIDEO_H_

/**
 * @file video.h
 * @brief Basic boot-time video routines
 * 
 * @defgroup video Basic video driver
 * @ingroup hal
*/

#include <stdint.h>
#include "defines.h"
#include "rtl/color.h"
#include <stdarg.h>

/**
 * @addtogroup video
 * @{
*/

/**
 * @brief (Re)Initialize boot-time video driver
 * @warning Should be deinitialized with HalVideoDeinit() when proper display driver is loaded.
*/
INTERNAL STATUS HalVideoInit(void);

/**
 * @brief Set background color
 * @param color RGB color
*/
INTERNAL void HalVideoSetBackgroundColor(RtlRGB color);

/**
 * @brief Set foreground (text) color
 * @param color RGB color
*/
INTERNAL void HalVideoSetForegroundColor(RtlRGB color);

/**
 * @brief Fill screen with given color
 * @param color RGB color
*/
INTERNAL void HalVideoFillScreen(RtlRGB color);

/**
 * @brief Clear screen (fill with current background color)
*/
INTERNAL void HalVideoClearScreen(void);

/**
 * @brief Put formatted string starting at given position
 * @param x Horizontal position
 * @param y Vertical position
 * @param *format Format string
 * @param args Argument list
 */
INTERNAL void HalVideoPrintVXY(uint16_t x, uint16_t y, const char *format, va_list args);

/**
 * @brief Put formatted string starting at given position
 * @param x Horizontal position
 * @param y Vertical position
 * @param *format Format string
 * @param ... Formatting arguments
 */
INTERNAL void HalVideoPrintXY(uint16_t x, uint16_t y, const char *format, ...);

/**
 * @brief Put formatted string
 * @param *format Format string
 * @param args Argument list
 */
INTERNAL void HalVideoPrintV(const char *format, va_list args);

/**
 * @brief Put formatted string
 * @param *format Format string
 * @param ... Formatting arguments
 */
INTERNAL void HalVideoPrint(const char *format, ...);

/**
 * @brief Set colors
 * @param fg Foreground (text) RGB color
 * @param bg Background color
*/
INTERNAL void HalVideoSetColor(RtlRGB fg, RtlRGB bg);

/**
 * @brief Set current cursor position
 * @param x X position in pixels
 * @param y Y position in pixels
*/
INTERNAL void HalVideoSetPosition(uint16_t x, uint16_t y);

/**
 * @brief Print character
 * @param c Character to print
*/
INTERNAL void HalVideoPrintChar(char c);

/**
 * @brief Set given pixel
 * @param x X position in pixels
 * @param y Y position in pixels
 * @param color Pixel RGB color
*/
INTERNAL void HalVideoSetPixel(uint16_t x, uint16_t y, RtlRGB color);

/**
 * @brief Display a bitmap
 * @param x Starting X position in pixels (top left corner)
 * @param y Starting Y position in pixels (top left corner)
 * @param *bitmap Bitmap array: line by line (horizontal scan)
 * @param width Bitmap width in pixels
 * @param height Bitmap height in pixels
*/
INTERNAL void HalVideoDisplayBitmap(uint16_t x, uint16_t y, const RtlRGB *bitmap, uint16_t width, uint16_t height);

/**
 * @brief Check whether boot-time video driver is available
 * @return True if boot-time video driver is available, false otherwise
 */
INTERNAL bool HalVideoIsAvailable(void);

EXPORT_API

/**
 * @brief Deinitialize boot-time video driver - gain ownership of the video adapter
*/
void HalVideoDeinit(void);

END_EXPORT_API


/**
 * @}
*/

#endif