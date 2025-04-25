#ifndef TTY_VT_H_
#define TTY_VT_H_

#include "defines.h"
#include <stdint.h>
#include "io/video/output.h"
#include "keymap.h"

struct IoEventHandler;
union IoEventData;

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
    } output;
    struct
    {
        uint32_t modifiers; /**< Modifier key bit field */
        int handle; /**< Input device handle */
    } input;
};

/**
 * @brief Process VT input - system event callback
 * @param *handler Event handler
 * @param *data Event data
 */
void TtyProcessVtInput(const struct IoEventHandler *handler, const union IoEventData *data);

#endif