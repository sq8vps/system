/**
 * @file video.h
 * @brief Basic boot-time video output abstraction layer
 * @ingroup hal
*/

#ifndef HAL_VIDEO_H_
#define HAL_VIDEO_H_

#include <stdint.h>
#include "defines.h"
#include "rtl/color.h"
#include <stdarg.h>

/**
 * @addtogroup hal_video Boot-time video output abstraction layer
 * @brief Boot-time video output abstraction layer
 * @kinternal
 * 
 * This module defines an abstraction layer for a boot-time video driver. If video output is not available, the routines are no-ops.
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
 * @brief Put null-terminated string starting at given position
 * @param x Horizontal position
 * @param y Vertical position
 * @param *s Null-terminated string
 */
INTERNAL void HalVideoPrintXY(size_t x, size_t y, const char *s);

/**
 * @brief Put null-termninated string
 * @param *s Null-terminated string
 */
INTERNAL void HalVideoPrint(const char *s);

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
INTERNAL void HalVideoSetPosition(size_t x, size_t y);

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
INTERNAL void HalVideoSetPixel(size_t x, size_t y, RtlRGB color);

/**
 * @brief Display a bitmap
 * @param x Starting X position in pixels (top left corner)
 * @param y Starting Y position in pixels (top left corner)
 * @param *bitmap Bitmap array: line by line (horizontal scan)
 * @param width Bitmap width in pixels
 * @param height Bitmap height in pixels
*/
INTERNAL void HalVideoDisplayBitmap(size_t x, size_t y, const RtlRGB *bitmap, size_t width, size_t height);

/**
 * @brief Check whether boot-time video driver is available
 * @return True if boot-time video driver is available, false otherwise
 */
INTERNAL bool HalVideoIsAvailable(void);

DRIVER_API

/**
 * @brief Deinitialize boot-time video driver - gain ownership of the video adapter
*/
void HalVideoDeinit(void);

/**
 * @brief Type of the reset video callback provided by the driver
 */
typedef STATUS (*HalVideoResetRoutine)(void *context);

/**
 * @brief Register video driver reset routine
 * @param *resetRoutine Pointer to the reset routine
 * @param *context Context pointer passed to the reset routine
 * 
 * The reset routine is used by the kernel to reset the video adapter to a known state,
 * where it can be set up and used by the kernel video driver.
 * The reset routine is invoked only on kernel panic to print the panic message without relying
 * on the external video driver.
 */
void HalRegisterVideoResetRoutine(HalVideoResetRoutine resetRoutine, void *context);

END_DRIVER_API


/**
 * @}
*/

#endif