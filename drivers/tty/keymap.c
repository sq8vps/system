#include "keymap.h"

/**
 * @brief Generic no-modifier keymap
 * @note This is just straight ASCII with non-printable symbols nulled
 */
static const char TtyKeymapAsciiLowercase[] = "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
    " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~\0";

/**
 * @brief Generic shift-only modified keymap
 */
static const char TtyKeymapAsciiUppercase[] = "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
    " !\"#$%&'()*+<_>?)!@#$%^&*(::<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ{|}^_~ABCDEFGHIJKLMNOPQRSTUVWXYZ{|}~\0";

/**
 * @brief Generic caps-only keymap
 * @note Caps lock affects only letters, making them uppercase
 */
static const char TtyKeymapAsciiCaps[] = "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
    " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`ABCDEFGHIJKLMNOPQRSTUVWXYZ{|}~\0";

/**
 * @brief Generic caps+shift keymap
 * @note Caps lock + shift acts as a shift on non-letters and does not affect letters
 */
static const char TtyKeymapAsciiCapsShift[] = "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
    " !\"#$%&'()*+<_>?)!@#$%^&*(::<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ{|}^_~abcdefghijklmnopqrstuvwxyz{|}~\0";

/**
 * @brief Modified keymap LUT
 */
static char* TtyKeymapModifiers[TTY_KEYMAP_MODIFIERS] = {NULL};

void TtyInitializeDefaultKeymap(void)
{
    for(size_t i = 0; i < TTY_KEYMAP_MODIFIERS; i++)
    {
        TtyKeymapModifiers[i] = NULL;
    }

    TtyKeymapModifiers[0] = TtyKeymapAsciiLowercase;
    TtyKeymapModifiers[TTY_KEYMAP_LEFT_SHIFT] = TtyKeymapAsciiUppercase;
    TtyKeymapModifiers[TTY_KEYMAP_RIGHT_SHIFT] = TtyKeymapAsciiUppercase;
    TtyKeymapModifiers[TTY_KEYMAP_LEFT_SHIFT | TTY_KEYMAP_RIGHT_SHIFT] = TtyKeymapAsciiUppercase;
    TtyKeymapModifiers[TTY_KEYMAP_CAPS] = TtyKeymapAsciiCaps;
    TtyKeymapModifiers[TTY_KEYMAP_LEFT_SHIFT | TTY_KEYMAP_CAPS] = TtyKeymapAsciiCapsShift;
    TtyKeymapModifiers[TTY_KEYMAP_RIGHT_SHIFT | TTY_KEYMAP_CAPS] = TtyKeymapAsciiCapsShift;
    TtyKeymapModifiers[TTY_KEYMAP_LEFT_SHIFT | TTY_KEYMAP_RIGHT_SHIFT | TTY_KEYMAP_CAPS] = TtyKeymapAsciiCapsShift;
}

const char *TtyDecodeKey(IoKeyCode code, uint8_t modifiers)
{
    if(unlikely(code > IO_KEY_ASCII_LAST))
        return NULL;

    if(unlikely(modifiers > (TTY_KEYMAP_MODIFIERS - 1)))
        return NULL;

    if(NULL == TtyKeymapModifiers[modifiers])
        return NULL;
    
    return TtyKeymapModifiers[modifiers] + code;
}
