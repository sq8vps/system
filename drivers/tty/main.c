#include "logging.h"
#include "device.h"
#include "io/dev/rp.h"
#include "io/dev/dev.h"
#include "ex/kdrv/kdrv.h"
#include "font.h"
#include "mm/heap.h"

static STATUS TtyDispatch(struct IoRp *rp)
{
    STATUS status = OK;

    struct TtyDeviceData *info = rp->device->privateData;

    switch(rp->code)
    {
        case IO_RP_OPEN:
            
            break;
        case IO_RP_CLOSE:
            status = OK;
            break;
        case IO_RP_READ:
            if(NULL == info->queue.read)
                status = NOT_SUPPORTED;
            else
            {
                IoMarkRpPending(rp);
                return IoStartRp(info->queue.read, rp, NULL);
            }
            break;    
        case IO_RP_WRITE:
            if(NULL == info->queue.write)
                status = NOT_SUPPORTED;
            else
            {
                IoMarkRpPending(rp);
                return IoStartRp(info->queue.write, rp, NULL);
            }
            break;
        case IO_RP_TERMINAL_CONTROL:
            status = TtyHandleControl(rp);
            break;
        default:
            status = BAD_PARAMETER;
            break;
    }

    rp->status = status;
    IoFinalizeRp(rp);
    return OK;
}

STATUS DRIVER_ENTRY(struct ExDriverObject *driverObject, const char *dbPath)
{
    UNUSED(dbPath);
    driverObject->dispatch = TtyDispatch;
    driverObject->addDevice = NULL;
    TtyLoggingInit();
    TtyInitializeDefaultKeymap();

    struct TtyDeviceData *info = MmAllocateKernelHeapZeroed(sizeof(*info));
    if(NULL == info)
        return OUT_OF_RESOURCES;

    info->type = TTY_TYPE_DUMMY;
    //this should result in creation of dummy /dev/ttyM0
    return TtyCreateDevice(driverObject, TTY_TYPE_DUMMY, info);
}

