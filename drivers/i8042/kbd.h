#ifndef I8042_KBD_H_
#define I8042_KBD_H_

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Key state flag: set if pressed (make), reset if released (break)
 */
#define PS2_KEY_STATE_BIT 0x8000

/**
 * @brief Key pulse flag: set if key is pulsed. If set, PS2_KET_STATE_BIT is meaningless
 */
#define PS2_KEY_PULSE_BIT 0x4000

/**
 * @brief Mask to extract key code from value returned from Ps2KeyboardParse
 */
#define PS2_KEY_VALUE_MASK ~(PS2_KEY_STATE_BIT | PS2_KEY_PULSE_BIT)

/**
 * @brief Value returned by Ps2KeyboardParse when read is still incomplete
 */
#define PS2_KEY_INCOMPLETE 0xFFFF

/**
 * @brief Keyboard LED state bits
 */
enum Ps2KeyboardLed
{
    PS2_KBD_LED_SCROLL_LOCK = 0x1,
    PS2_KBD_LED_NUM_LOCK = 0x2,
    PS2_KBD_LED_CAPS_LOCK = 0x4,
};

struct I8042Peripheral;
struct IoRp;

/**
 * @brief Parse keyboard scan code
 * @param data Received PS/2 byte
 * @param *buffer Buffer of size at least 8 for internal use
 * @param *index Index for internal use
 * @return Decoded code as in IoKeyMappings, optionally with PS2_KEY_STATE_BIT or PS2_KEY_PULSE_BIT.
 * PS2_KEY_INCOMPLETE is returned when key parsing is in progress
 */
uint16_t Ps2KeyboardParse(uint8_t data, uint8_t *buffer, uint8_t *index);

/**
 * @brief Set up keyboard synchronously
 * @param *info i8042 peripheral info
 */
bool Ps2KeyboardSetUp(struct I8042Peripheral *info);

/**
 * @brief Set LEDs on keyboard asynchronously
 * @param state LED state to be set
 * @param *rp Associated Request Packet
 */
void Ps2KeyboardSetLeds(enum Ps2KeyboardLed state, struct IoRp *rp);

#endif