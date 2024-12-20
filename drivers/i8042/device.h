#ifndef I8042_DEVICE_H_
#define I8042_DEVICE_H_

#include "defines.h"
#include <stdint.h>
#include <stdbool.h>
#include "ke/core/mutex.h"
#include "io/dev/res.h"

struct I8042Controller
{
    uint32_t initialized : 1; /**< Controller was initialized */
    struct
    {
        uint32_t first : 1; /**< First port is usable */
        uint32_t second : 1; /**< Second port is usable */
    } usable;

    struct
    {
        uint32_t first : 1; /**< First port is populated */
        uint32_t second : 1; /**< Second port is populated */
    } present;

    KeSpinlock lock; /**< Structure lock */
};

enum Ps2DeviceType
{
    PS2_UNKNOWN = 0,
    PS2_MOUSE,
};

struct I8042Peripheral
{
    bool port; /**< False if first port, true if second */
    enum Ps2DeviceType type; /**< Device type */
};

extern struct I8042Controller I8042ControllerInfo;

/**
 * @brief Initialize Intel 8042 PS/2 controller
 * @return Status code
 */
STATUS I8042InitializeController(void);

/**
 * @brief Write to PS/2 peripheral
 * @param device False to select the first device, true to select the second device
 * @param data Data to send
 * @return True on success, false on timeout or if the device is not available
 */
bool I8042WriteToPeripheral(bool device, uint8_t data);

#endif