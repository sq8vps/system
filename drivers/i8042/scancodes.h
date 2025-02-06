#ifndef I8042_SCANCODES_H_
#define I8042_SCANCODES_H_

#include <stdint.h>
#include "io/input/kbd.h"

/**
 * @brief Scan code set 2 key mappings for 101-, 102-, 104-key US QWERTY keyboards
 */
static uint16_t Ps2UsSet2[] = 
{
    0, //empty
    IO_KEY_F9,
    0, //empty
    IO_KEY_F5,
    IO_KEY_F3,
    IO_KEY_F1,
    IO_KEY_F2,
    IO_KEY_F12,
    0, //empty
    IO_KEY_F10,
    IO_KEY_F8,
    IO_KEY_F6,
    IO_KEY_F4,
    '\t',
    '`',
    0, 0, //empty
    IO_KEY_LEFT_ALT,
    IO_KEY_LEFT_SHIFT,
    0, //empty
    IO_KEY_LEFT_CTRL,
    'Q',
    '1',
    0, 0, 0, //empty
    'Z',
    'S',
    'A',
    'W',
    '2',
    0, //system left, extended only
    0, //empty
    'C',
    'X',
    'D',
    'E',
    '4',
    '3',
    0, //system right, extended only
    0, //empty
    ' ',
    'V',
    'F',
    'T',
    'R',
    '5',
    0, //context menu, extended only
    0, //empty
    'N',
    'B',
    'H',
    'G',
    'Y',
    '6',
    0, 0, 0, //empty
    'M',
    'J',
    'U',
    '7',
    '8',
    0, 0, //empty
    ',',
    'K',
    'I',
    'O',
    '0',
    '9',
    0, 0, //empty
    '.',
    '/',
    'L',
    ';',
    'P',
    '-',
    0, 0, 0, //empty
    '\'',
    0, //empty
    '[',
    '=',
    0, 0, //empty
    IO_KEY_CAPS_LOCK,
    IO_KEY_RIGHT_SHIFT,
    IO_KEY_ENTER,
    ']',
    0, //empty
    '\\',
    0, 0, 0, 0, 0, 0, 0, 0, //empty
    '\b',
    0, 0, //empty
    IO_KEY_KEYPAD_1,
    0, //empty
    IO_KEY_KEYPAD_4,
    IO_KEY_KEYPAD_7,
    0, 0, 0, //empty
    IO_KEY_KEYPAD_0,
    IO_KEY_KEYPAD_DOT,
    IO_KEY_KEYPAD_2,
    IO_KEY_KEYPAD_5,
    IO_KEY_KEYPAD_6,
    IO_KEY_KEYPAD_8,
    IO_KEY_ESC,
    IO_KEY_NUM_LOCK,
    IO_KEY_F11,
    IO_KEY_KEYPAD_PLUS,
    IO_KEY_KEYPAD_3,
    IO_KEY_KEYPAD_MINUS,
    IO_KEY_KEYPAD_ASTERISK,
    IO_KEY_KEYPAD_9,
    IO_KEY_SCROLL_LOCK,
    0, 0, 0, 0, //empty
    IO_KEY_F7,
};

/**
 * @brief Extended (0xE0) scan code set 2 key mappings for 101-, 102-, 104-key US QWERTY keyboards
 */
static uint16_t Ps2UsSet2Ext[] =
{
    [16] = IO_KEY_WWW_SEARCH,
    [17] = IO_KEY_RIGHT_ALT,
    [20] = IO_KEY_RIGHT_CTRL,
    [21] = IO_KEY_PREVIOUS_TRACK,
    [24] = IO_KEY_WWW_FAVORTIES,
    [31] = IO_KEY_LEFT_SYSTEM,
    [32] = IO_KEY_WWW_REFRESH,
    [33] = IO_KEY_VOLUME_DOWN,
    [35] = IO_KEY_MUTE,
    [39] = IO_KEY_RIGHT_SYSTEM,
    [40] = IO_KEY_WWW_STOP,
    [43] = IO_KEY_CALCULATOR,
    [47] = IO_KEY_MENU,
    [48] = IO_KEY_WWW_FORWARD,
    [50] = IO_KEY_VOLUME_UP,
    [52] = IO_KEY_PLAY_PAUSE,
    [55] = IO_KEY_POWER,
    [56] = IO_KEY_WWW_BACK,
    [58] = IO_KEY_WWW_HOME,
    [59] = IO_KEY_STOP,
    [63] = IO_KEY_SLEEP,
    [64] = IO_KEY_MY_COMPUTER,
    [72] = IO_KEY_E_MAIL,
    [74] = IO_KEY_KEYPAD_SLASH,
    [77] = IO_KEY_NEXT_TRACK,
    [80] = IO_KEY_MEDIA_SELECT,
    [90] = IO_KEY_KEYPAD_ENTER,
    [94] = IO_KEY_WAKE,
    [105] = IO_KEY_END,
    [107] = IO_KEY_LEFT_ALT,
    [108] = IO_KEY_HOME,
    [112] = IO_KEY_INSERT,
    [113] = 0xFF, //ASCII DEL
    [114] = IO_KEY_DOWN_ARROW,
    [116] = IO_KEY_RIGHT_ARROW,
    [117] = IO_KEY_UP_ARROW,
    [122] = IO_KEY_PAGE_DOWN,
    [125] = IO_KEY_PAGE_UP,
};


#endif