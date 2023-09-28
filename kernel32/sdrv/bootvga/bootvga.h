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
 * 4. This driver cannot be invoked with any standard drawing/printing routines
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
STATUS BootVgaInit(void);

/**
 * @brief Deinitialize boot VGA driver
*/
void BootVgaDeinit(void);

/**
 * @brief Set background color
 * @param color RGB color
*/
void BootVgaSetBackgroundColor(CmRGB color);

/**
 * @brief Set foreground (text) color
 * @param color RGB color
*/
void BootVgaSetForegroundColor(CmRGB color);

/**
 * @brief Fill screen with given color
 * @param color RGB color
*/
void BootVgaFillScreen(CmRGB color);

/**
 * @brief Clear screen (fill with current background color)
*/
void BootVgaClearScreen(void);

/**
 * @brief Print string at given location
 * @param *s String to print
 * @param x X position in pixels
 * @param y Y position in pixels
 * @attention X=0, Y=0 is the top left corner
*/
void BootVgaPrintStringXY(uint16_t x, uint16_t y, char *s);

/**
 * @brief Print string
 * @param *s String to print
*/
void BootVgaPrintString(char *s);

/**
 * @brief Set colors
 * @param fg Foreground (text) RGB color
 * @param bg Background color
*/
void BootVgaSetColor(CmRGB fg, CmRGB bg);

/**
 * @brief Set current cursor position
 * @param x X position in pixels
 * @param y Y position in pixels
*/
void BootVgaSetPosition(uint16_t x, uint16_t y);

/**
 * @brief Print character
 * @param c Character to print
*/
void BootVgaPrintChar(char c);

/**
 * @brief Get current screen resolution
 * @param *x Width in pixels
 * @param *y Height in pixels
*/
void BootVgaGetCurrentResolution(uint16_t *x, uint16_t *y);

/**
 * @brief Set given pixel
 * @param x X position in pixels
 * @param y Y position in pixels
 * @param color Pixel RGB color
*/
void BootVgaSetPixel(uint16_t x, uint16_t y, CmRGB color);

/**
 * @brief Display a bitmap
 * @param x Starting X position in pixels (top left corner)
 * @param y Starting Y position in pixels (top left corner)
 * @param *bitmap Bitmap array: line by line (horizontal scan)
 * @param width Bitmap width in pixels
 * @param height Bitmap height in pixels
*/
void BootVgaDisplayBitmap(uint16_t x, uint16_t y, CmRGB *bitmap, uint16_t width, uint16_t height);

/**
 * @}
*/

#endif