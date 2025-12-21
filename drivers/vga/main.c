#include "logging.h"
#include "io/dev/rp.h"
#include "io/dev/dev.h"
#include "ex/kdrv/kdrv.h"
#include "mm/heap.h"
#include "vga.h"
#include "vesa.h"
#include "hal/video.h"

/**
 * @brief Request dispatch routine
*/
static STATUS VgaDispatch(struct IoRp *rp)
{
    struct IoDeviceObject *dev = IoGetCurrentRpPosition(rp);
    if(NULL == dev->privateData)
        return NOT_SUPPORTED;

    switch(rp->code)
    {
        //enumerate displays
        case IO_RP_ENUMERATE:
            VgaEnumerateDisplays(rp);
            return OK;
            break;
        case IO_RP_GET_DEVICE_ID:
            if(VGA_INFO_ADAPTER == *((enum VgaInfoType*)dev->privateData))
            {
                return IoSendRpDown(rp);
            }
            else if(VGA_INFO_DISPLAY == *((enum VgaInfoType*)dev->privateData))
            {
                VgaGetDisplayDeviceId(rp);
                return OK;
            }
            else
            {
                rp->status = NOT_IMPLEMENTED;
                IoFinalizeRp(rp);
                break;
            }
            break;
        default:
            rp->status = BAD_PARAMETER;
            IoFinalizeRp(rp);
            break;
    }
    return OK;
}

static STATUS VgaInit(struct ExDriverObject *driverObject)
{
    return OK;
} 

/**
 * @brief Add VGA adapter
*/
static STATUS VgaAddDevice(struct ExDriverObject *driverObject, struct IoDeviceObject *baseDeviceObject)
{
    struct IoDeviceObject *device = NULL;
    STATUS status = OK;

    if(OK != (status = IoCreateDevice(driverObject, IO_DEVICE_TYPE_VIDEO, 0, &device)))
        return status;

    struct VgaAdapterInfo *info = MmAllocateKernelHeapZeroed(sizeof(*info));
    if(NULL == info)
    {
        IoDestroyDevice(device);
        return OUT_OF_RESOURCES;
    }

    info->type = VGA_INFO_ADAPTER;
    info->resetMode = 0xFFFF;
    
    device->privateData = info;
    device->flags |= IO_DEVICE_FLAG_ENUMERATION_CAPABLE;

    status = VgaVesaGetAdapterInfo(info);
    if(OK != status)
    {
        MmFreeKernelHeap(info);
        IoDestroyDevice(device);
        return status;
    }

    IoAttachDevice(device, baseDeviceObject);

    HalRegisterVideoResetRoutine(VgaResetAdapter, info);
        
    return OK;
}

/**
 * @brief Main driver entry routine, called only once when the driver is loaded to the memory
*/
STATUS DRIVER_ENTRY(struct ExDriverObject *driverObject)
{
    driverObject->init = VgaInit;
    driverObject->dispatch = VgaDispatch;
    driverObject->addDevice = VgaAddDevice;
    VgaLoggingInit();
    return OK;
}

