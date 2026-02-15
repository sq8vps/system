/**
 * @file tty.h
 * @brief Terminal DDK and helpers
 * @ingroup ddk
 */

#ifndef DDK_TTY_H_
#define DDK_TTY_H_

#include <stdint.h>
#include <stdbool.h>
#include "defines.h"

DRIVER_API

struct IoDeviceObject;


/**
 * @addtogroup ddk_tty Terminal requests and helpers
 * @ingroup ddk
 * @{
 */

NABLA_API

/**
 * @brief TTY device name length limit
 */
#define TTY_DEVICE_NAME_SIZE 64

END_NABLA_API

/**
 * @brief TTY-driver specific operations
 */
enum TtyOperations
{
    TTY_NONE = 0, /**< No operation */
    TTY_CREATE_VT = 1, /**< Create virtual terminal */
};

/**
 * @brief Parameters for TTY-specific requests
 */
struct TtyParameters
{
    /**
     * @brief Request-specific data
     */
    union
    {
        /**
         * @brief Data for the @ref TTY_CREATE_VT request
         */
        struct
        {
            int inputEvent; /**< Event generator (keyboard) handle. -1 to disable input */
            int outputDisplay; /** Output display handle. -1 to disable output */
            char name[TTY_DEVICE_NAME_SIZE + 1]; /**< Created device name */
        } createVt;
    } request;
};

END_DRIVER_API

NABLA_API

/**
 * @brief Create a new virtual terminal
 * @param masterHandle Handle of the master TTY (VT-capable) device file
 * @param inputEvent Input event handle (-1 to disable input)
 * @param outputEvent Output even handle (-1 to disable output)
 * @param *name Array to store the new VT name to. Must be able to hold at least @ref TTY_DEVICE_NAME_SIZE + 1 characters. 
 * Might be set to NULL if not needed.
 * @return Status code
 * @note @a name contains only the name of the device and is not a full path (e.g., does not include \a \dev\...)
 */
STATUS ApiCreateVt(int masterHandle, int inputEvent, int outputEvent, char *name);

END_NABLA_API

/**
 * @brief Create new virtual terminal
 * @param *dev Target terminal device
 * @param *params Request parameters/response data
 * @return Status code
 */
INTERNAL STATUS TtyCreateVt(struct IoDeviceObject *dev, struct TtyParameters *params);

/**
 * @}
 */

#endif