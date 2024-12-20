#include "io/dev/dev.h"
#include "rtl/string.h"
#include "logging.h"
#include "device.h"
#include "ps2.h"

static STATUS I8042Dispatch(struct IoRp *rp)
{
    STATUS status;

    struct IoDeviceObject *dev = IoGetCurrentRpPosition(rp);

    switch(rp->code)
    {
        default:
            status = RP_PROCESSING_FAILED;
            break;
    }

    rp->status = status;
    IoFinalizeRp(rp);
    return OK;
}

STATUS I8042AddDevice(struct ExDriverObject *driverObject, struct IoDeviceObject *baseDeviceObject)
{
    struct IoDeviceObject *dev = NULL;
    STATUS status = OK;

    PRIO prio = KeAcquireSpinlock(&(I8042ControllerInfo.lock));
    if(I8042ControllerInfo.present.first && I8042ControllerInfo.present.second)
    {
        KeReleaseSpinlock(&(I8042ControllerInfo.lock), prio);
        return FILE_ALREADY_EXISTS;
    }

    if(!I8042ControllerInfo.initialized)
    {
        status = I8042InitializeController();
        if(OK != status)
        {
            KeReleaseSpinlock(&(I8042ControllerInfo.lock), prio);
            return status;
        }
        I8042ControllerInfo.initialized = 1;
    }

    status = IoCreateDevice(driverObject, IO_DEVICE_TYPE_INPUT, 0, &dev);
    if(OK != status)
    {
        KeReleaseSpinlock(&(I8042ControllerInfo.lock), prio);
        return status;
    }

    dev->privateData = MmAllocateKernelHeapZeroed(sizeof(struct I8042Peripheral));
    if(NULL == dev)
    {
        IoDestroyDevice(dev);
        return OUT_OF_RESOURCES;
    }
    
    struct I8042Peripheral *info = dev->privateData;
    info->type = PS2_UNKNOWN;

    if(I8042ControllerInfo.usable.first && !I8042ControllerInfo.present.first)
    {
        info->port = false;
        I8042ControllerInfo.present.first = Ps2ProbePort(info);
    }
    else if(I8042ControllerInfo.usable.second && !I8042ControllerInfo.present.second)
    {
        info->port = true;
        I8042ControllerInfo.present.second = Ps2ProbePort(info);
    }

    KeReleaseSpinlock(&(I8042ControllerInfo.lock), prio);

    IoAttachDevice(dev, baseDeviceObject);
    
    return OK;
I8042AddDeviceFailure:
    MmFreeKernelHeap(dev->privateData);
    IoDestroyDevice(dev);
    return status;
}

static STATUS I8042Init(struct ExDriverObject *driverObject)
{
    return OK;
} 


STATUS DRIVER_ENTRY(struct ExDriverObject *driverObject)
{
    driverObject->init = I8042Init;
    driverObject->dispatch = I8042Dispatch;
    driverObject->addDevice = I8042AddDevice;
    I8042LoggingInit();
    return OK;
}

