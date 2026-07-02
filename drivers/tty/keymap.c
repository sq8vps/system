#include "keymap.h"
#include "rtl/string.h"
#include "vt.h"
#include "io/input/kbd.h"

/*
The arrays below contain basic UTF-8 compatible keymap-to-character mappings.
Each entry ends with NULL, which allows to store multiple bytes of a single UTF-8 character
or a UTF-8 character sequence. 
These are used ONLY when no other keymap is specified.
*/

/**
 * @brief Generic no-modifier keymap
 */
static const char TtyUtf8KeymapAsciiLowercase[IO_PRINTABLE_KEY_COUNT * 2] =
    "a\0b\0c\0d\0e\0f\0g\0h\0i\0j\0k\0l\0m\0n\0o\0p\0q\0r\0s\0t\0u\0v\0w\0x\0y\0z\0"
    "0\0""1\0""2\0""3\0""4\0""5\0""6\0""7\0""8\0""9\0"
    " \0-\0=\0[\0]\0\\\0;\0'\0,\0.\0/\0`";

/**
 * @brief Generic shift-only modified keymap
 */
static const char TtyUtf8KeymapAsciiUppercase[IO_PRINTABLE_KEY_COUNT * 2] = 
    "A\0B\0C\0D\0E\0F\0G\0H\0I\0J\0K\0L\0M\0N\0O\0P\0Q\0R\0S\0T\0U\0V\0W\0X\0Y\0Z\0"
    ")\0!\0@\0#\0$\0%\0^\0&\0*\0(\0"
    " \0_\0+\0{\0}\0|\0:\0\"\0<\0>\0?\0~";

/**
 * @brief Generic caps-only keymap
 * @note Caps lock affects only letters, making them uppercase
 */
static const char TtyUtf8KeymapAsciiCaps[IO_PRINTABLE_KEY_COUNT * 2] =
    "A\0B\0C\0D\0E\0F\0G\0H\0I\0J\0K\0L\0M\0N\0O\0P\0Q\0R\0S\0T\0U\0V\0W\0X\0Y\0Z\0"
    "0\0""1\0""2\0""3\0""4\0""5\0""6\0""7\0""8\0""9\0"
    " \0-\0=\0[\0]\0\\\0;\0'\0,\0.\0/\0`";

/**
 * @brief Generic caps+shift keymap
 * @note Caps lock + shift acts as a shift on non-letters and does not affect letters
 */
static const char TtyUtf8KeymapAsciiCapsShift[IO_PRINTABLE_KEY_COUNT * 2] =
    "a\0b\0c\0d\0e\0f\0g\0h\0i\0j\0k\0l\0m\0n\0o\0p\0q\0r\0s\0t\0u\0v\0w\0x\0y\0z\0"
    ")\0!\0@\0#\0$\0%\0^\0&\0*\0(\0"
    " \0_\0+\0{\0}\0|\0:\0\"\0<\0>\0?\0~";

/**
 * @brief Generic control character keymap
 * @note This only includes control characters that map directly to ASCII control characters, such as tab, backspace, delete, and enter.
 */
static const char TtyUtf8KeymapAsciiControl[8] = 
    "\t\0\b\0\b\0\r";

/*
The arrays below point to corresponding UTF-8-encoded characters or UTF-8 sequences for given key code.
These are used ONLY when no other keymap is specified.
*/

/**
 * @brief Generic no-modifier keymap
 */
static const char* TtyKeymapAsciiLowercase[IO_KEY_COUNT];

/**
 * @brief Generic shift-only modified keymap
 */
static const char* TtyKeymapAsciiUppercase[IO_KEY_COUNT];

/**
 * @brief Generic caps-only keymap
 * @note Caps lock affects only letters, making them uppercase
 */
static const char* TtyKeymapAsciiCaps[IO_KEY_COUNT];

/**
 * @brief Generic caps+shift keymap
 * @note Caps lock + shift acts as a shift on non-letters and does not affect letters
 */
static const char* TtyKeymapAsciiCapsShift[IO_KEY_COUNT];

/**
 * @brief Keymap for different combinations of modifiers
 */
static const char** TtyKeymap[TTY_KEYMAP_MODIFIERS];

void TtyInitializeDefaultKeymap(void)
{
    RtlMemset(TtyKeymap, 0, sizeof(TtyKeymap));
    RtlMemset(TtyKeymapAsciiLowercase, 0, sizeof(TtyKeymapAsciiLowercase));
    RtlMemset(TtyKeymapAsciiUppercase, 0, sizeof(TtyKeymapAsciiUppercase));
    RtlMemset(TtyKeymapAsciiCaps, 0, sizeof(TtyKeymapAsciiCaps));
    RtlMemset(TtyKeymapAsciiCapsShift, 0, sizeof(TtyKeymapAsciiCapsShift));

    for(size_t i = 0; i < IO_PRINTABLE_KEY_COUNT; i++)
    {
        TtyKeymapAsciiLowercase[i] = &TtyUtf8KeymapAsciiLowercase[i * 2];
        TtyKeymapAsciiUppercase[i] = &TtyUtf8KeymapAsciiUppercase[i * 2];
        TtyKeymapAsciiCaps[i] = &TtyUtf8KeymapAsciiCaps[i * 2];
        TtyKeymapAsciiCapsShift[i] = &TtyUtf8KeymapAsciiCapsShift[i * 2];
    }

    for(size_t i = 0; i < sizeof(TtyUtf8KeymapAsciiControl) / 2; i++)
    {
        TtyKeymapAsciiLowercase[i + IO_FIRST_CONTROL_KEY] = &TtyUtf8KeymapAsciiControl[i * 2];
        TtyKeymapAsciiUppercase[i + IO_FIRST_CONTROL_KEY] = &TtyUtf8KeymapAsciiControl[i * 2];
        TtyKeymapAsciiCaps[i + IO_FIRST_CONTROL_KEY] = &TtyUtf8KeymapAsciiControl[i * 2];
        TtyKeymapAsciiCapsShift[i + IO_FIRST_CONTROL_KEY] = &TtyUtf8KeymapAsciiControl[i * 2];
    }

    TtyKeymap[0] = TtyKeymapAsciiLowercase;
    TtyKeymap[TTY_KEYMAP_LEFT_SHIFT] = TtyKeymapAsciiUppercase;
    TtyKeymap[TTY_KEYMAP_RIGHT_SHIFT] = TtyKeymapAsciiUppercase;
    TtyKeymap[TTY_KEYMAP_LEFT_SHIFT | TTY_KEYMAP_RIGHT_SHIFT] = TtyKeymapAsciiUppercase;
    TtyKeymap[TTY_KEYMAP_CAPS] = TtyKeymapAsciiCaps;
    TtyKeymap[TTY_KEYMAP_LEFT_SHIFT | TTY_KEYMAP_CAPS] = TtyKeymapAsciiCapsShift;
    TtyKeymap[TTY_KEYMAP_RIGHT_SHIFT | TTY_KEYMAP_CAPS] = TtyKeymapAsciiCapsShift;
    TtyKeymap[TTY_KEYMAP_LEFT_SHIFT | TTY_KEYMAP_RIGHT_SHIFT | TTY_KEYMAP_CAPS] = TtyKeymapAsciiCapsShift;
}

const char *TtyDecodeKey(IoKeyCode code, uint8_t modifiers)
{
    if(unlikely(code >= IO_KEY_COUNT))
        return NULL;

    if(unlikely(modifiers > (TTY_KEYMAP_MODIFIERS - 1)))
        return NULL;

    if(NULL == TtyKeymap[modifiers])
        return NULL;

    if(NULL == TtyKeymap[modifiers][code])
        return NULL;
    
    return TtyKeymap[modifiers][code];
}
