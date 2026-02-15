/**
 * @file kbd.h
 * @brief Keyboard support
 * @ingroup io_input
 */

#ifndef KERNEL_KBD_H_
#define KERNEL_KBD_H_

#include "defines.h"
#include <stdint.h>

/**
 * @addtogroup io_input_kbd Keyboard support
 * @brief Keyboard support definition and routines
 * @ingroup io_input
 * @{
 */

DRIVER_API

struct IoDeviceObject;
struct IoEventHandler;

/**
 * @brief I/O input subsystem keyboard key code type
 */
typedef uint16_t IoKeyCode;


/**
 * @brief Mapping of keys to key codes
 */
enum IoKeyMapping
{
    IO_FIRST_PRINTABLE_KEY = 0, /**< First printable key code. Not a real key. */

    IO_KEY_A = IO_FIRST_PRINTABLE_KEY,
    IO_KEY_B,
    IO_KEY_C,
    IO_KEY_D,
    IO_KEY_E,
    IO_KEY_F,
    IO_KEY_G,
    IO_KEY_H,
    IO_KEY_I,
    IO_KEY_J,
    IO_KEY_K,
    IO_KEY_L,
    IO_KEY_M,
    IO_KEY_N,
    IO_KEY_O,
    IO_KEY_P,
    IO_KEY_Q,
    IO_KEY_R,
    IO_KEY_S,
    IO_KEY_T,
    IO_KEY_U,
    IO_KEY_V,
    IO_KEY_W,
    IO_KEY_X,
    IO_KEY_Y,
    IO_KEY_Z,
    IO_KEY_0, /**< 0 */
    IO_KEY_1, /**< 1 */
    IO_KEY_2, /**< 2 */
    IO_KEY_3, /**< 3 */
    IO_KEY_4, /**< 4 */
    IO_KEY_5, /**< 5 */
    IO_KEY_6, /**< 6 */
    IO_KEY_7, /**< 7 */
    IO_KEY_8, /**< 8 */
    IO_KEY_9, /**< 9 */
    IO_KEY_SPACE, /**< Space */
    IO_KEY_MINUS, /**< Minus */
    IO_KEY_EQUAL, /**< Equal */
    IO_KEY_LEFT_BRACKET, /**< Left bracket */
    IO_KEY_RIGHT_BRACKET, /**< Right bracket */
    IO_KEY_BACKSLASH, /**< Backslash */
    IO_KEY_SEMICOLON, /**< Semicolon */
    IO_KEY_APOSTROPHE, /**< Apostrophe */
    IO_KEY_COMMA, /**< Comma */
    IO_KEY_DOT, /**< Dot */
    IO_KEY_SLASH, /**< Slash */
    IO_KEY_GRAVE, /**< Grave accent */

    IO_PRINTABLE_KEY_COUNT = IO_KEY_GRAVE - IO_FIRST_PRINTABLE_KEY + 1, /**< Count of printable keys */
    IO_FIRST_CONTROL_KEY = IO_PRINTABLE_KEY_COUNT, /**< First control key code. Not a real key. */

    IO_KEY_TAB = IO_FIRST_CONTROL_KEY, /**< Tab */
    IO_KEY_BACKSPACE, /**< Backspace */
    IO_KEY_DELETE, /**< Delete */
    IO_KEY_ENTER, /**< Enter */
    IO_KEY_ESCAPE , /**< Escape */
    IO_KEY_LEFT_ALT, /**< Left alt */
    IO_KEY_RIGHT_ALT, /**<Right alt */
    IO_KEY_LEFT_SHIFT, /**< Left shift */
    IO_KEY_RIGHT_SHIFT, /**< Right shift */
    IO_KEY_LEFT_CTRL, /**< Left control */
    IO_KEY_RIGHT_CTRL, /**< Right control */
    IO_KEY_LEFT_SYSTEM, /**< Left system key */
    IO_KEY_RIGHT_SYSTEM, /**< Right system key */
    IO_KEY_MENU, /**< Context menu key */
    IO_KEY_CAPS_LOCK, /**< Caps lock */
    IO_KEY_END, /**< End */
    IO_KEY_LEFT_ARROW, /**< Left arrow */
    IO_KEY_RIGHT_ARROW, /**< Right arrow */
    IO_KEY_UP_ARROW, /**< Up arrow */
    IO_KEY_DOWN_ARROW, /**< Down arrow */
    IO_KEY_HOME, /**< Home */
    IO_KEY_INSERT, /**< Insert */
    IO_KEY_NUM_LOCK, /**< Num lock */
    IO_KEY_SCROLL_LOCK, /**< Scroll lock */
    IO_KEY_PAGE_UP, /**< Page up */
    IO_KEY_PAGE_DOWN, /**< Page down */
    IO_KEY_PRINT_SCREEN, /**< Print screen */
    IO_KEY_PAUSE, /**< Pause/break */
    IO_KEY_F1, /**< F1 */
    IO_KEY_F2, /**< F2 */
    IO_KEY_F3, /**< F3 */
    IO_KEY_F4, /**< F4 */
    IO_KEY_F5, /**< F5 */
    IO_KEY_F6, /**< F6 */
    IO_KEY_F7, /**< F7 */
    IO_KEY_F8, /**< F8 */
    IO_KEY_F9, /**< F9 */
    IO_KEY_F10, /**< F10 */
    IO_KEY_F11, /**< F11 */
    IO_KEY_F12, /**< F12 */
    IO_KEY_POWER, /**< Power key */
    IO_KEY_SLEEP, /**< Sleep key */
    IO_KEY_WAKE, /**< Wake key */
    IO_KEY_NEXT_TRACK, /**< Next track */
    IO_KEY_PREVIOUS_TRACK, /**< Previous track */
    IO_KEY_STOP, /**< Multimedia stop */
    IO_KEY_PLAY_PAUSE, /**< Multimedia play/pause */
    IO_KEY_MUTE, /**< Multimedia mute */
    IO_KEY_VOLUME_UP, /**< Volume up */
    IO_KEY_VOLUME_DOWN, /**< Volume down */
    IO_KEY_MEDIA_SELECT, /**< Media select */
    IO_KEY_E_MAIL, /**< E-mail */
    IO_KEY_CALCULATOR, /**< Calculator */
    IO_KEY_MY_COMPUTER, /**< My computer */
    IO_KEY_WWW_SEARCH, /**< WWW search */
    IO_KEY_WWW_HOME, /**< WWW home */
    IO_KEY_WWW_BACK, /**< WWW back */
    IO_KEY_WWW_FORWARD, /**< WWW forward */
    IO_KEY_WWW_STOP, /**< WWW stop */
    IO_KEY_WWW_REFRESH, /**< WWW refresh */
    IO_KEY_WWW_FAVORTIES, /**< WWW favorites */
    IO_KEY_KEYPAD_ENTER, /**< Keypad enter */
    IO_KEY_KEYPAD_SLASH, /**< Keypad slash */
    IO_KEY_KEYPAD_ASTERISK, /**< Keypad asterisk */
    IO_KEY_KEYPAD_MINUS, /**< Keypad minus */
    IO_KEY_KEYPAD_PLUS, /**< Keypad plus */
    IO_KEY_KEYPAD_DOT, /**< Keypad dot */
    /*
     * Keypad numerical keys. Starting from IO_KEY_KEYPAD_0, all numerical keys have consecutive codes.
     */
    IO_KEY_KEYPAD_0, /**< Keypad 0 */
    IO_KEY_KEYPAD_1, /**< Keypad 1 */
    IO_KEY_KEYPAD_2, /**< Keypad 2 */
    IO_KEY_KEYPAD_3, /**< Keypad 3 */
    IO_KEY_KEYPAD_4, /**< Keypad 4 */
    IO_KEY_KEYPAD_5, /**< Keypad 5 */
    IO_KEY_KEYPAD_6, /**< Keypad 6 */
    IO_KEY_KEYPAD_7, /**< Keypad 7 */
    IO_KEY_KEYPAD_8, /**< Keypad 8 */
    IO_KEY_KEYPAD_9, /**< Keypad 9 */

    IO_CONTROL_KEY_COUNT = IO_KEY_KEYPAD_9 - IO_FIRST_CONTROL_KEY + 1, /**< Number of control key mappings */
    IO_KEY_COUNT = IO_PRINTABLE_KEY_COUNT + IO_CONTROL_KEY_COUNT, /**< Total number of key mappings */
};

/**
 * @brief Data for keyboard events
 */
struct IoKeyboardEventData
{
    IoKeyCode key; /**< Key code */
    bool state; /**< Key state: true if pressed, false if released */
};

/**
 * @brief Report key state change to the input subsystem
 * @param handle Input device handle obtained from IoRegisterInputDevice()
 * @param code Key code. Key code is equal to ASCII code or to a value from IoKeyMapping
 * @param pressed True if key was pressed, false if key was released
 * @return Status code
 */
STATUS IoKeyboardReportKey(int handle, IoKeyCode code, bool pressed);

END_DRIVER_API

/**
 * @}
 */

#endif