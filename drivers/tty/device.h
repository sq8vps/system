#ifndef TTY_DEVICE_H_
#define TTY_DEVICE_H_

#include "defines.h"
#include <stdbool.h>
#include "ddk/tty.h"
#include "vt.h"
#include "rtl/ring.h"
#include "ke/core/mutex.h"

struct ExDriverObject;
struct IoRpQueue;
struct IoRp;

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
    struct
    {
        struct IoRpQueue *write; /**< TTY write RP queue */
        struct IoRpQueue *read; /**< TTY read RP queue */
    } queue;
    union
    {
        struct TtyVtData vt; /**< VT-associated data */
    };

    struct
    {
        char *buffer; /**< Input (device -> program) buffer */
        size_t lines; /**< Number of complete lines in the input buffer */
        struct RingBuffer ring; /**< Input ring buffer */
        struct IoRp *currentRp; /**< Current read request */
        KeSpinlock lock; /**< Spinlock for the "input" structure */
    } input;
};

STATUS TtyCreateDevice(struct ExDriverObject *drv, enum TtyType type, struct TtyDeviceData *info);

STATUS TtyHandleControl(struct IoRp *rp);

/**
 * @brief Write string to device
 * @param *tty Pointer to the TTY device data structure
 * @param *data Data to write
 * @param size Size of the data
 */
void TtyPutString(struct TtyDeviceData *tty, const char *data, size_t size);

/**
 * @brief Process input data from device
 * @param *tty Pointer to the TTY device data structure
 * @param *data Data to process
 * @param size Size of the data
 */
void TtyProcessInput(struct TtyDeviceData *tty, const char *data, size_t size);

#endif