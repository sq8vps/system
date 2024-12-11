#ifndef TTY_DEVICE_H_
#define TTY_DEVICE_H_

#include "defines.h"
#include <stdbool.h>

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
    enum TtyType type;
    bool activated;
    struct
    {
        struct IoRpQueue *write;
    } queue;
};

STATUS TtyCreateDevice(struct ExDriverObject *drv, enum TtyType type, uint32_t *id);

#endif