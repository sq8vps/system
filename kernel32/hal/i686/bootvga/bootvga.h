#ifndef KERNEL_BOOTVGA_H_
#define KERNEL_BOOTVGA_H_

/**
 * @file bootvga.h
 * @brief Boot VGA display driver
 * 
 * This module provides a very simple VGA driver for use in following cases:
 * 1. During early boot stages when there are no proper display drivers loaded yet
 * 2. For displaying bug check informations in panic routines
 * Keep in mind that:
 * 1. This driver should NOT be used to show any kernel output.
 * 2. This driver should be deinitialized as soon as the proper display driver is loaded
 * 3. This driver is a core system driver and does not provide a typical loadable driver interface
 * 
 * Currently supports 320x200 and 640x480 resolutions in graphic mode and "emulated" text mode using built-in font.
 * 
 * @note The resolution and color depth are hardcoded.
 * @note Quite heavily based on this public domain code: https://files.osdev.org/mirrors/geezer/osd/graphics/modes.c 
 * and Windows NT 5.1 bootvid.dll driver source code.
 * 
 * @defgroup bootVga Boot VGA display driver
*/

#include <stdint.h>
#include "defines.h"
#include "color.h"

/**
 * @addtogroup bootVga
 * @{
*/

/**
 * @brief (Re)Initialize boot VGA driver
 * @warning Should be deinitialized with BootVgaDeinit() when proper display driver is loaded.
*/
INTERNAL STATUS BootVgaInit(void);

/**
 * @brief Deinitialize boot VGA driver
*/
INTERNAL void BootVgaDeinit(void);

/**
 * @brief Set background color
 * @param color RGB color
*/
INTERNAL void BootVgaSetBackgroundColor(CmRGB color);

/**
 * @brief Set foreground (text) color
 * @param color RGB color
*/
INTERNAL void BootVgaSetForegroundColor(CmRGB color);

/**
 * @brief Fill screen with given color
 * @param color RGB color
*/
INTERNAL void BootVgaFillScreen(CmRGB color);

/**
 * @brief Clear screen (fill with current background color)
*/
INTERNAL void BootVgaClearScreen(void);

/**
 * @brief Print string at given location
 * @param x X position in pixels
 * @param y Y position in pixels
 * @param *s String to print
 * @attention X=0, Y=0 is the top left corner
*/
INTERNAL void BootVgaPrintStringXY(uint16_t x, uint16_t y, char *s);

/**
 * @brief Print up to \a size characters of a string at given location
 * @param x X position in pixels
 * @param y Y position in pixels
 * @param *s String to print
 * @param size Maximum number of characters
 * @attention X=0, Y=0 is the top left corner
*/
INTERNAL void BootVgaPrintStringNXY(uint16_t x, uint16_t y, char *s, uint32_t size);

/**
 * @brief Print up to \a size characters of a string
 * @param *s String to print
 * @param size Maximum number of characters
*/
INTERNAL void BootVgaPrintStringN(char *s, uint32_t size);

/**
 * @brief Print string
 * @param *s String to print
*/
INTERNAL void BootVgaPrintString(char *s);

/**
 * @brief Set colors
 * @param fg Foreground (text) RGB color
 * @param bg Background color
*/
INTERNAL void BootVgaSetColor(CmRGB fg, CmRGB bg);

/**
 * @brief Set current cursor position
 * @param x X position in pixels
 * @param y Y position in pixels
*/
INTERNAL void BootVgaSetPosition(uint16_t x, uint16_t y);

/**
 * @brief Print character
 * @param c Character to print
*/
INTERNAL void BootVgaPrintChar(char c);

/**
 * @brief Get current screen resolution
 * @param *x Width in pixels
 * @param *y Height in pixels
*/
INTERNAL void BootVgaGetCurrentResolution(uint16_t *x, uint16_t *y);

/**
 * @brief Set given pixel
 * @param x X position in pixels
 * @param y Y position in pixels
 * @param color Pixel RGB color
*/
INTERNAL void BootVgaSetPixel(uint16_t x, uint16_t y, CmRGB color);

/**
 * @brief Display a bitmap
 * @param x Starting X position in pixels (top left corner)
 * @param y Starting Y position in pixels (top left corner)
 * @param *bitmap Bitmap array: line by line (horizontal scan)
 * @param width Bitmap width in pixels
 * @param height Bitmap height in pixels
*/
INTERNAL void BootVgaDisplayBitmap(uint16_t x, uint16_t y, CmRGB *bitmap, uint16_t width, uint16_t height);

/**
 * @}
*/

#endif