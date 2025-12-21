#ifndef TTY_VT_H_
#define TTY_VT_H_

#include "defines.h"
#include <stdint.h>
#include "io/video/output.h"
#include "keymap.h"

struct IoEventHandler;
union IoEventData;
struct ExDriverObject;
struct TtyParameters;
struct TtyFont;
struct TtyVtData;

typedef uint8_t TtyColor[3]; //TTY RGB color
typedef uint64_t TtyVtPixel;

/**
 * @brief Keyboard modifiers
 */
enum
{
    TTY_VT_LEFT_CTRL = TTY_KEYMAP_LEFT_CTRL,
    TTY_VT_RIGHT_CTRL = TTY_KEYMAP_RIGHT_CTRL,
    TTY_VT_LEFT_ALT = TTY_KEYMAP_LEFT_ALT,
    TTY_VT_RIGHT_ALT = TTY_KEYMAP_RIGHT_ALT,
    TTY_VT_LEFT_SHIFT = TTY_KEYMAP_LEFT_SHIFT,
    TTY_VT_RIGHT_SHIFT = TTY_KEYMAP_RIGHT_SHIFT,
    TTY_VT_CAPS = TTY_KEYMAP_CAPS,

    TTY_VT_LEFT_SYSTEM = TTY_VT_CAPS * 2,
    TTY_VT_RIGHT_SYSTEM = TTY_VT_LEFT_SYSTEM * 2,
    TTY_VT_MENU = TTY_VT_RIGHT_SYSTEM * 2,
};

struct TtyVtData
{
    struct
    {
        union IoVideoOutput config; /**< Output device configuration */
        enum IoVideoType type; /**< Output device type */
        int handle; /**< Output device handle */

        struct
        {
            uint32_t x; /**< Number of columns */
            uint32_t y; /**< Number of rows */
        } limits;
        struct
        {
            uint32_t x; /**< Cursor X position */
            uint32_t y; /**< Cursor Y position */
        } cursor;

        TtyColor fg; /**< Foreground color */
        TtyColor bg; /**< Background color */
        size_t bytesPerPixel; /**< Bytes per pixel */
        TtyVtPixel fgPixel; /** Pre-prepared foreground pixel */
        TtyVtPixel bgPixel; /**< Pre-prepared background pixel */
        TtyVtPixel mask; /**< Pre-prepared pixel mask */
        
        struct TtyFont *font; /**< Pointer to the font structure */
    } output;
    struct
    {
        uint32_t modifiers; /**< Modifier key bit field */
        int handle; /**< Input device handle */
        char** modifierMap[TTY_KEYMAP_MODIFIERS]; /**< Modifier-to-appropriate-keymap LUT */
    } input;
};


/**
 * @brief Create a virtual terminal
 * @param *drv Driver object
 * @param *params TTY parameters
 * @return Status code
 */
STATUS TtyCreateVt(struct ExDriverObject *drv, struct TtyParameters *params);

/**
 * @brief Write string to VT
 * @param *info Pointer to the TTY VT data structure
 * @param *str Pointer to the data to write
 * @param size Size of the data to write
 * @return Status code
 */
void TtyPutVtString(struct TtyVtData *info, const char *str, size_t len);

/**
 * @brief Set VT foreground and background colors
 * @param *vt Pointer to the TTY VT data structure
 * @param fg Foreground color
 * @param bg Background color
 */
void TtyVtSetColor(struct TtyVtData *vt, TtyColor fg, TtyColor bg);

#endif