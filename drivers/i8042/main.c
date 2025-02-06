#include "io/dev/dev.h"
#include "rtl/string.h"
#include "logging.h"
#include "device.h"
#include "ps2.h"
#include "io/dev/rp.h"
#include "ex/kdrv/kdrv.h"
#include "mm/heap.h"
#include "io/input/input.h"

static struct IoRpQueue *I8042RpQueue = NULL;

static void I8042ProcessRp(struct IoRp *rp)
{

}

static STATUS I8042Dispatch(struct IoRp *rp)
{
    STATUS status;

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
    if(I8042ControllerInfo.port[PORT_FIRST].present && I8042ControllerInfo.port[PORT_SECOND].present)
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
        status = IoCreateRpQueue(I8042ProcessRp, &I8042RpQueue);
        if(OK != status)
        {
            KeReleaseSpinlock(&(I8042ControllerInfo.lock), prio);
            return status;
        }
        I8042ControllerInfo.initialized = 1;
    }

    status = IoCreateDevice(driverObject, IO_DEVICE_TYPE_OTHER, 0, &dev);
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
    info->device = dev;

    if(I8042ControllerInfo.port[PORT_FIRST].usable && !I8042ControllerInfo.port[PORT_FIRST].present)
    {
        I8042ControllerInfo.port[PORT_FIRST].device = info;
        info->port = PORT_FIRST;
        I8042ControllerInfo.port[PORT_FIRST].present = Ps2ProbePort(info);
    }
    else if(I8042ControllerInfo.port[PORT_SECOND].usable && !I8042ControllerInfo.port[PORT_SECOND].present)
    {
        I8042ControllerInfo.port[PORT_SECOND].device = info;
        info->port = PORT_SECOND;
        I8042ControllerInfo.port[PORT_SECOND].present = Ps2ProbePort(info);
    }

    KeReleaseSpinlock(&(I8042ControllerInfo.lock), prio);

    status = IoRegisterInputDevice(dev, &(info->handle));
    if(OK != status)
    {
        MmFreeKernelHeap(info);
        IoDestroyDevice(dev);
        return status;
    }

    IoAttachDevice(dev, baseDeviceObject);
    
    return OK;
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

