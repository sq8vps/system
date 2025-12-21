#ifndef I8042_SCANCODES_H_
#define I8042_SCANCODES_H_

#include <stdint.h>
#include "io/input/kbd.h"

#define PS2_UNKNOWN_KEY 0xFFFF

/**
 * @brief Scan code set 2 key mappings for 101-, 102-, 104-key US QWERTY keyboards
 */
static uint16_t Ps2UsSet2[] = 
{
    PS2_UNKNOWN_KEY, //empty
    IO_KEY_F9,
    0, //empty
    IO_KEY_F5,
    IO_KEY_F3,
    IO_KEY_F1,
    IO_KEY_F2,
    IO_KEY_F12,
    PS2_UNKNOWN_KEY, //empty
    IO_KEY_F10,
    IO_KEY_F8,
    IO_KEY_F6,
    IO_KEY_F4,
    IO_KEY_TAB,
    IO_KEY_GRAVE,
    PS2_UNKNOWN_KEY, PS2_UNKNOWN_KEY, //empty
    IO_KEY_LEFT_ALT,
    IO_KEY_LEFT_SHIFT,
    PS2_UNKNOWN_KEY, //empty
    IO_KEY_LEFT_CTRL,
    IO_KEY_Q,
    IO_KEY_1,
    PS2_UNKNOWN_KEY, PS2_UNKNOWN_KEY, PS2_UNKNOWN_KEY, //empty
    IO_KEY_Z,
    IO_KEY_S,
    IO_KEY_A,
    IO_KEY_W,
    IO_KEY_2,
    PS2_UNKNOWN_KEY, //system left, extended only
    PS2_UNKNOWN_KEY, //empty
    IO_KEY_C,
    IO_KEY_X,
    IO_KEY_D,
    IO_KEY_E,
    IO_KEY_4,
    IO_KEY_3,
    PS2_UNKNOWN_KEY, //system right, extended only
    PS2_UNKNOWN_KEY, //empty
    IO_KEY_SPACE,
    IO_KEY_V,
    IO_KEY_F,
    IO_KEY_T,
    IO_KEY_R,
    IO_KEY_5,
    PS2_UNKNOWN_KEY, //context menu, extended only
    PS2_UNKNOWN_KEY, //empty
    IO_KEY_N,
    IO_KEY_B,
    IO_KEY_H,
    IO_KEY_G,
    IO_KEY_Y,
    IO_KEY_6,
    PS2_UNKNOWN_KEY, PS2_UNKNOWN_KEY, PS2_UNKNOWN_KEY, //empty
    IO_KEY_M,
    IO_KEY_J,
    IO_KEY_U,
    IO_KEY_7,
    IO_KEY_8,
    PS2_UNKNOWN_KEY, PS2_UNKNOWN_KEY, //empty
    IO_KEY_COMMA,
    IO_KEY_K,
    IO_KEY_I,
    IO_KEY_O,
    IO_KEY_0,
    IO_KEY_9,
    PS2_UNKNOWN_KEY, PS2_UNKNOWN_KEY, //empty
    IO_KEY_DOT,
    IO_KEY_SLASH,
    IO_KEY_L,
    IO_KEY_SEMICOLON,
    IO_KEY_P,
    IO_KEY_MINUS,
    PS2_UNKNOWN_KEY, PS2_UNKNOWN_KEY, PS2_UNKNOWN_KEY, //empty
    IO_KEY_APOSTROPHE,
    PS2_UNKNOWN_KEY, //empty
    IO_KEY_LEFT_BRACKET,
    IO_KEY_EQUAL,
    PS2_UNKNOWN_KEY, PS2_UNKNOWN_KEY, //empty
    IO_KEY_CAPS_LOCK,
    IO_KEY_RIGHT_SHIFT,
    IO_KEY_ENTER,
    IO_KEY_RIGHT_BRACKET,
    PS2_UNKNOWN_KEY, //empty
    IO_KEY_BACKSLASH,
    PS2_UNKNOWN_KEY, PS2_UNKNOWN_KEY, PS2_UNKNOWN_KEY, PS2_UNKNOWN_KEY, PS2_UNKNOWN_KEY, PS2_UNKNOWN_KEY, PS2_UNKNOWN_KEY, PS2_UNKNOWN_KEY, //empty
    IO_KEY_BACKSPACE,
    PS2_UNKNOWN_KEY, PS2_UNKNOWN_KEY, //empty
    IO_KEY_KEYPAD_1,
    PS2_UNKNOWN_KEY, //empty
    IO_KEY_KEYPAD_4,
    IO_KEY_KEYPAD_7,
    PS2_UNKNOWN_KEY, PS2_UNKNOWN_KEY, PS2_UNKNOWN_KEY, //empty
    IO_KEY_KEYPAD_0,
    IO_KEY_KEYPAD_DOT,
    IO_KEY_KEYPAD_2,
    IO_KEY_KEYPAD_5,
    IO_KEY_KEYPAD_6,
    IO_KEY_KEYPAD_8,
    IO_KEY_ESCAPE, //escape
    IO_KEY_NUM_LOCK,
    IO_KEY_F11,
    IO_KEY_KEYPAD_PLUS,
    IO_KEY_KEYPAD_3,
    IO_KEY_KEYPAD_MINUS,
    IO_KEY_KEYPAD_ASTERISK,
    IO_KEY_KEYPAD_9,
    IO_KEY_SCROLL_LOCK,
    PS2_UNKNOWN_KEY, PS2_UNKNOWN_KEY, PS2_UNKNOWN_KEY, PS2_UNKNOWN_KEY, //empty
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
    [113] = IO_KEY_DELETE,
    [114] = IO_KEY_DOWN_ARROW,
    [116] = IO_KEY_RIGHT_ARROW,
    [117] = IO_KEY_UP_ARROW,
    [122] = IO_KEY_PAGE_DOWN,
    [125] = IO_KEY_PAGE_UP,
};


#endif