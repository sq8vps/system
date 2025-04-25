#ifndef TTY_KEYMAP_H_
#define TTY_KEYMAP_H_

#include "defines.h"
#include "io/input/kbd.h"

/**
 * @brief Keymap modifiers
 */
enum TtyKeymapModifiers
{
    TTY_KEYMAP_LEFT_CTRL = 1,
    TTY_KEYMAP_RIGHT_CTRL = 2,
    TTY_KEYMAP_LEFT_ALT = 4,
    TTY_KEYMAP_RIGHT_ALT = 8,
    TTY_KEYMAP_LEFT_SHIFT = 16,
    TTY_KEYMAP_RIGHT_SHIFT = 32,
    TTY_KEYMAP_CAPS = 64,
};

/**
 * @brief Number of keymap modifiers combinations
 */
#define TTY_KEYMAP_MODIFIERS (TTY_KEYMAP_CAPS * 2)

/**
 * @brief Keymap modifiers mask
 */
#define TTY_KEYMAP_MODIFIERS_MASK ((TTY_KEYMAP_MODIFIERS) - 1)

/**
 * @brief Initialize default keymap
 */
void TtyInitializeDefaultKeymap(void);

/**
 * @brief Decode key and return pointer to the respective UTF-8 character
 * @param code Key code
 * @param modifiers Key modifiers
 * @return Pointer to the UTF-8 character or NULL if not found
 */
const char *TtyDecodeKey(IoKeyCode code, uint8_t modifiers);

#endif