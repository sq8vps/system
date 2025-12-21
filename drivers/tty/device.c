#include "device.h"
#include "ex/kdrv/kdrv.h"
#include "io/dev/dev.h"
#include "io/fs/devfs.h"
#include "mm/heap.h"
#include <stdatomic.h>
#include "rtl/stdio.h"
#include "rtl/string.h"
#include "io/dev/rp.h"
#include "write.h"
#include "read.h"
#include "vt.h"

#define TTY_BUFFER_SIZE 4096 /**< TTY input buffer size */

const char *TtyTypePrefix[] = 
{
    [TTY_TYPE_DUMMY] = "M",
    [TTY_TYPE_VT] = "",
};

#define TTY_DEVICE_NAME_PREFIX "tty"
_Atomic uint32_t TtyDeviceIndex[TTY_TYPE_COUNT] = {[0 ... TTY_TYPE_COUNT - 1] = 0};

STATUS TtyCreateDevice(struct ExDriverObject *drv, enum TtyType type, struct TtyDeviceData *info)
{
    struct IoDeviceObject *dev = NULL;
    struct IoRpQueue *writeQueue = NULL;
    struct IoRpQueue *readQueue = NULL;
    STATUS status = OK;

    info->input.buffer = MmAllocateKernelHeap(TTY_BUFFER_SIZE);
    info->input.lines = 0;
    if(NULL == info->input.buffer)
        return OUT_OF_RESOURCES;

    RingBufferInitialize(&info->input.ring, TTY_BUFFER_SIZE);
    info->input.currentRp = NULL;
    info->input.lock = (KeSpinlock)KeSpinlockInitializer;

    status = IoCreateDevice(drv, IO_DEVICE_TYPE_TERMINAL, IO_DEVICE_FLAG_STANDALONE | IO_DEVICE_FLAG_BUFFERED_IO, &dev);
    if(OK == status)
    {
        dev->alignment = 0;
        dev->blockSize = 0;
        dev->privateData = info;

        status = IoCreateRpQueue(TtyWrite, &writeQueue);
        if(OK != status)
            goto TtyCreateDeviceFailed;
        
        info->queue.write = writeQueue;

        status = IoCreateRpQueue(TtyRead, &readQueue);
        if(OK != status)
            goto TtyCreateDeviceFailed;
        
        info->queue.read = readQueue;

        status = IoRegisterStandaloneDevice(dev);
        if(OK != status)
            goto TtyCreateDeviceFailed;

        info->type = type;

        uint32_t ttyId = atomic_fetch_add_explicit(&(TtyDeviceIndex[type]), 1, memory_order_relaxed);

        snprintf(info->name, sizeof(info->name), TTY_DEVICE_NAME_PREFIX "%s%lu", TtyTypePrefix[type], ttyId);
        status = IoCreateDeviceFile(dev, 0, info->name);
        if(OK != status)
            return status;
    }

    return status;

TtyCreateDeviceFailed:
    if((NULL == dev) || (NULL == dev->node.deviceNode))
    {
        if(NULL != writeQueue)
            IoDestroyRpQueue(writeQueue);
        if(NULL != dev)
            IoDestroyDevice(dev);
    }

    MmFreeKernelHeap(info->input.buffer);
    info->input.buffer = NULL;
        
    return status;
}


STATUS TtyHandleControl(struct IoRp *rp)
{
    if(unlikely(IO_RP_TERMINAL_CONTROL != rp->code))
        return BAD_PARAMETER;
    
    switch(rp->payload.deviceControl.code)
    {
        case TTY_CREATE_VT:
            return TtyCreateVt(rp->device->driverObject, rp->payload.deviceControl.data);
            break;
        default:
            return NOT_IMPLEMENTED;
            break;
    }
}

void TtyPutString(struct TtyDeviceData *tty, const char *data, size_t size)
{
    if(TTY_TYPE_VT == tty->type)
    {
        TtyPutVtString(&tty->vt, data, size);
    }
}

void TtyProcessInput(struct TtyDeviceData *tty, const char *data, size_t size)
{
    //FIXME: local echo doesn't account for empty or full buffer...
    size_t toPrint = 0;

    PRIO prio = KeAcquireDpcLevelSpinlock(&tty->input.lock);

    for(size_t i = 0; i < size; i++)
    {
        switch(data[i])
        {
            case '\n':
            case '\r':
                if(0 == RingBufferGetFree(&tty->input.ring))
                    break;

                RingBufferPush(&tty->input.ring, tty->input.buffer, '\n');
                ++tty->input.lines;
                ++toPrint;

                if(NULL != tty->input.currentRp)
                {
                    struct IoRp *rp = tty->input.currentRp;
                    size_t s = (rp->size > RingBufferGetSize(&tty->input.ring)) ? 
                        RingBufferGetSize(&tty->input.ring) : rp->size;

                    if(likely(0 != s))
                    {
                        for(size_t k = 0; k < s; k++)
                        {
                            ((char*)rp->payload.read.systemBuffer)[k] = RingBufferPop(&tty->input.ring, tty->input.buffer);
                        }

                        if('\n' == ((char*)rp->payload.read.systemBuffer)[s - 1])
                            --tty->input.lines;
                    }

                    rp->size = s;
                    rp->status = OK;
                    tty->input.currentRp = NULL;
                    KeReleaseSpinlock(&tty->input.lock, prio);
                    IoFinalizeRp(rp);
                    prio = KeAcquireDpcLevelSpinlock(&tty->input.lock);
                }

                break;
            case '\b':
                if((0 != RingBufferGetSize(&tty->input.ring)) && ('\n' != RingBufferPeek(&tty->input.ring, tty->input.buffer)))
                {
                    (void)RingBufferPop(&tty->input.ring, tty->input.buffer);
                    ++toPrint;
                }
                break;
            case '\t':
            case '\v':
            case '\f':
            case '\a':
            default:
                if(RingBufferGetFree(&tty->input.ring) <= 1) //leave space for enter
                    break;
                RingBufferPush(&tty->input.ring, tty->input.buffer, data[i]);
                ++toPrint;
                break;
        }
    }

    KeReleaseSpinlock(&tty->input.lock, prio);

    TtyPutString(tty, data, toPrint);
}

