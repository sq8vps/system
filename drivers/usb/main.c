#include "logging.h"
#include "io/dev/rp.h"
#include "io/dev/dev.h"
#include "ex/kdrv/kdrv.h"
#include "mm/heap.h"
#include "rtl/string.h"
#include "hc.h"

/**
 * @brief Request dispatch routine
*/
static STATUS UhciDispatch(struct IoRp *rp)
{
    STATUS status = OK;

    struct IoDeviceObject *dev = IoGetCurrentRpPosition(rp);

    switch(rp->code)
    {
        case IO_RP_ENUMERATE:
            
            break;
        case IO_RP_READ:
        case IO_RP_WRITE:
            
            break;
        case IO_RP_GET_DEVICE_ID:
            break;
        default:
            rp->status = BAD_PARAMETER;
            IoFinalizeRp(rp);
            break;
    }
    return OK;
}

static STATUS UhciInit(struct ExDriverObject *driverObject)
{
    return OK;
} 

/**
 * @brief Add UHCI device object (MDO)
 * 
 * This function is called on the UHCI object to create a main device, i.e., the host controller.
 * It is not called for enumerated devices.
*/
static STATUS UhciAddDevice(struct ExDriverObject *driverObject, struct IoDeviceObject *baseDeviceObject)
{
    struct IoDeviceObject *device = nullptr;
    STATUS status = OK;

    status = IoCreateDevice(driverObject, IO_DEVICE_TYPE_STORAGE, 0, &device);
    if(OK != status)
        return status;

    struct UhciControllerInfo *info = MmAllocateKernelHeapZeroed(sizeof(*info));
    if(nullptr == info)
    {
        IoDestroyDevice(device);
        return OUT_OF_RESOURCES;
    }

    IoAttachDevice(device, baseDeviceObject);
        
    return UhciConfigureController(baseDeviceObject, device, info);
}

/**
 * @brief Main driver entry routine, called only once when the driver is loaded to the memory
*/
STATUS DRIVER_ENTRY(struct ExDriverObject *driverObject)
{
    driverObject->init = UhciInit;
    driverObject->dispatch = UhciDispatch;
    driverObject->addDevice = UhciAddDevice;
    UhciLoggingInit();
    return OK;
}

