#ifndef I8042_PS2_H_
#define I8042_PS2_H_

#include <stdint.h>
#include <stdbool.h>

struct I8042Peripheral;

/**
 * @brief Enable or disable scanning
 * @param *info i8042 peripheral data
 * @param state Enable state
 * @return True on success, false on timeout or if device not available
 */
bool Ps2SetScanningEnabled(struct I8042Peripheral *info, bool state);

/**
 * @brief Probe PS/2 port and initialize PS/2 device
 * @param *info i8042 peripheral data to fill
 * @return True if probed and initialized successfully
 */
bool Ps2ProbePort(struct I8042Peripheral *info);

#endif