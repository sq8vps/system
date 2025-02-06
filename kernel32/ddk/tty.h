#ifndef DDK_TTY_H_
#define DDK_TTY_H_

#include <stdint.h>
#include <stdbool.h>
#include "defines.h"

EXPORT_API

/**
 * @brief TTY device name length limit
 */
#define TTY_DEVICE_NAME_SIZE 64

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
    union request
    {
        /**
         * @brief Data for TTY_CREATE_VT request
         */
        struct createVt
        {
            int inputEvent; /**< Event generator (keyboard) handle. -1 to disable input */
            int outputDisplay; /** Output display handle. -1 to disable output */
            char name[TTY_DEVICE_NAME_SIZE + 1]; /**< Created device name */
        } createVt;
    } request;
};

END_EXPORT_API

/**
 * @brief Create new virtual terminal
 * @param *dev Target terminal device
 * @param *params Request parameters/response data
 * @return Status code
 */
INTERNAL STATUS TtyCreateVt(struct IoDeviceObject *dev, struct TtyParameters *params);

#endif