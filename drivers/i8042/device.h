#ifndef I8042_DEVICE_H_
#define I8042_DEVICE_H_

#include "defines.h"
#include <stdint.h>
#include <stdbool.h>
#include "ke/core/mutex.h"
#include "io/dev/res.h"

struct IoDeviceObject;

#define PORT_FIRST 0
#define PORT_SECOND 1

enum Ps2DeviceType
{
    PS2_UNKNOWN = 0,
    PS2_KEYBOARD,
    PS2_MOUSE,
};

struct I8042Peripheral
{
    struct IoDeviceObject *device; /**< System device object */
    uint8_t port; /**< PORT_FIRST or PORT_SECOND */
    enum Ps2DeviceType type; /**< Device type */
    int handle; /**< Kernel input device handle */
};

struct I8042Controller
{
    uint32_t initialized : 1; /**< Controller was initialized */

    struct
    {
        uint32_t usable : 1; /**< Port is usable */
        uint32_t present : 1; /**< Peripheral is present */
        struct I8042Peripheral *device; /**< Peripheral structure pointer */
        uint32_t gsi; /**< Global IRQ number */
    } port[2];

    KeSpinlock lock; /**< Structure lock */
};

extern struct I8042Controller I8042ControllerInfo;

/**
 * @brief Initialize Intel 8042 PS/2 controller
 * @return Status code
 */
STATUS I8042InitializeController(void);

/**
 * @brief Enable or disable IRQs
 * @param port PORT_FIRST or PORT_SECOND
 * @param state True to enable, false to disable
 */
void I8042SetInterruptsEnabled(uint8_t port, bool state);

/**
 * @brief Write to PS/2 peripheral
 * @param porte PORT_FIRST or PORT_SECOND
 * @param data Data to send
 * @return True on success, false on timeout or if the device is not available
 */
bool I8042WriteToPeripheral(uint8_t port, uint8_t data);

/**
 * @brief Read from PS/2 peripheral
 * @param *data Data buffer
 * @param count Number of bytes to read
 * @return True on success, false on timeout
 */
bool I8042ReadFromPeripheral(uint8_t *data, uint16_t count);

#endif