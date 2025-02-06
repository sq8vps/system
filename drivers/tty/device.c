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
#include "io/input/input.h"

const char *TtyTypePrefix[] = 
{
    [TTY_TYPE_DUMMY] = "",
    [TTY_TYPE_VT] = "",
};

#define TTY_DEVICE_NAME_PREFIX "tty"
_Atomic uint32_t TtyDeviceIndex[TTY_TYPE_COUNT] = {[0 ... TTY_TYPE_COUNT - 1] = 0};

STATUS TtyCreateDevice(struct ExDriverObject *drv, enum TtyType type, struct TtyDeviceData *info)
{
    struct IoDeviceObject *dev = NULL;
    struct IoRpQueue *writeQueue = NULL;
    STATUS status = OK;

    status = IoCreateDevice(drv, IO_DEVICE_TYPE_TERMINAL, IO_DEVICE_FLAG_STANDALONE | IO_DEVICE_FLAG_BUFFERED_IO, &dev);
    if(OK == status)
    {
        dev->alignment = 0;
        dev->blockSize = 0;

        status = IoCreateRpQueue(TtyWrite, &writeQueue);
        if(OK != status)
            goto TtyCreateDeviceFailed;
        
        info->queue.write = writeQueue;

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
        
    return status;
}

static STATUS TtyCreateVt(struct ExDriverObject *drv, struct TtyParameters *params)
{
    STATUS status = OK;

    IoRegisterEventHandler()

    struct TtyDeviceData *info = MmAllocateKernelHeapZeroed(sizeof(*info));
    if(NULL == info)
        return OUT_OF_RESOURCES;

    status = TtyCreateDevice(drv, TTY_TYPE_VT, info);
    if(OK != status)
    {
        MmFreeKernelHeap(info);
        return status;
    }

    RtlStrcpy(params->request.createVt.name, info->name);

    return status;
}

STATUS TtyHandleControl(struct IoRp *rp)
{
    if(unlikely(IO_RP_TERMINAL_CONTROL != rp->code))
        return RP_PROCESSING_FAILED;
    
    switch(rp->payload.deviceControl.code)
    {
        case TTY_CREATE_VT:

            break;
        default:
            return RP_CODE_UNKNOWN;
            break;
    }
}