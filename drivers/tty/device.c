#include "device.h"
#include "ex/kdrv/kdrv.h"
#include "io/dev/dev.h"
#include "io/fs/devfs.h"
#include "mm/heap.h"
#include <stdatomic.h>
#include "rtl/stdio.h"
#include "io/dev/rp.h"
#include "write.h"

const char *TtyTypePrefix[] = 
{
    [TTY_TYPE_DUMMY] = "",
    [TTY_TYPE_VT] = "",
};

#define TTY_DEVICE_NAME_PREFIX "tty"
_Atomic uint32_t TtyDeviceIndex[TTY_TYPE_COUNT] = {[0 ... TTY_TYPE_COUNT - 1] = 0};

STATUS TtyCreateDevice(struct ExDriverObject *drv, enum TtyType type, uint32_t *id)
{
    struct IoDeviceObject *dev = NULL;
    struct TtyDeviceData *info = NULL;
    struct IoRpQueue *writeQueue = NULL;
    STATUS status = OK;

    status = IoCreateDevice(drv, IO_DEVICE_TYPE_TERMINAL, IO_DEVICE_FLAG_STANDALONE | IO_DEVICE_FLAG_BUFFERED_IO, &dev);
    if(OK == status)
    {
        dev->alignment = 0;
        dev->blockSize = 0;
        dev->privateData = MmAllocateKernelHeapZeroed(sizeof(struct TtyDeviceData));
        if(NULL == dev->privateData)
        {
            status = OUT_OF_RESOURCES;
            goto TtyCreateDeviceFailed;
        }

        info = dev->privateData;

        status = IoCreateRpQueue(TtyWrite, &writeQueue);
        if(OK != status)
            goto TtyCreateDeviceFailed;
        
        info->queue.write = writeQueue;

        status = IoRegisterStandaloneDevice(dev);
        if(OK != status)
            goto TtyCreateDeviceFailed;

        info->type = type;
        info->activated = false;

        uint32_t ttyId = atomic_fetch_add_explicit(&(TtyDeviceIndex[type]), 1, memory_order_relaxed);

        char name[20];
        snprintf(name, sizeof(name), TTY_DEVICE_NAME_PREFIX "%s%lu", TtyTypePrefix[type], ttyId);
        status = IoCreateDeviceFile(dev, 0, name);
        if(OK != status)
            return status;

        if(NULL != id)
            *id = ttyId;
    }

    return status;

TtyCreateDeviceFailed:
    if((NULL == dev) || (NULL == dev->node.deviceNode))
    {
        if(NULL != writeQueue)
            IoDestroyRpQueue(writeQueue);
        if(NULL != info)
            MmFreeKernelHeap(info);
        if(NULL != dev)
            IoDestroyDevice(dev);
    }
        
    return status;
}