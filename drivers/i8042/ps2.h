#ifndef I8042_PS2_H_
#define I8042_PS2_H_

#include <stdint.h>
#include <stdbool.h>

struct I8042Peripheral;
struct IoRp;

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

/**
 * @brief Send data to PS/2 device synchronously
 * @param *info i8042 peripheral data
 * @param *data Data to send
 * @param count Size of data
 * @return True on success, false on failure
 */
bool Ps2WriteMultiple(struct I8042Peripheral *info, uint8_t *data, uint16_t count);

/**
 * @brief Send data to PS/2 device asynchronously
 * @param *info i8042 peripheral data
 * @param *data Data to send
 * @param size Size of data
 * @param *rp Request Packet associated with this request
 */
void Ps2SendCommand(struct I8042Peripheral *info, const void *data, uint8_t size, struct IoRp *rp);

/**
 * @brief Handle interrupt from PS/2 device
 * @param port PORT_FIRST or PORT_SECOND
 * @param data Received data byte
 */
void Ps2HandleIrq(uint8_t port, uint8_t data);

#endif