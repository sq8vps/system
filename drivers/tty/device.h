#ifndef TTY_DEVICE_H_
#define TTY_DEVICE_H_

#include "defines.h"
#include <stdbool.h>
#include "ddk/tty.h"

struct ExDriverObject;
struct IoRpQueue;

enum TtyType
{
    TTY_TYPE_DUMMY = 0,
    TTY_TYPE_VT = 1,

    TTY_TYPE_COUNT,
};

struct TtyDeviceData
{
    enum TtyType type; /**< TTY type */
    char name[TTY_DEVICE_NAME_SIZE]; /**< TTY device name */
    struct queue
    {
        struct IoRpQueue *write; /**< TTY write RP queue */
    } queue;
    int inputHandle, outputHandle; /**< VT input event and output display handle */
};

STATUS TtyCreateDevice(struct ExDriverObject *drv, enum TtyType type, uint32_t *id);

#endif