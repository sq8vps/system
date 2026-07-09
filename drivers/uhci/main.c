#include "logging.h"
#include "io/dev/rp.h"
#include "io/dev/dev.h"
#include "ex/kdrv/kdrv.h"
#include "mm/heap.h"
#include "rtl/string.h"
#include "hc.h"
#include "ex/db/db.h"
#include "if.h"

static struct ExDriverObject *UhciHighLevelDriver = nullptr;

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
    struct UsbHcInterface interface;

    if(nullptr == UhciHighLevelDriver)
    {
        LOG(SYSLOG_ERROR, "High level driver unknown!");
        return NOT_FOUND;
    }

    status = IoCreateDevice(driverObject, IO_DEVICE_TYPE_BUS, 0, &device);
    if(OK != status)
        return status;

    struct UhciControllerInfo *info = MmAllocateKernelHeapZeroed(sizeof(*info));
    if(nullptr == info)
    {
        IoDestroyDevice(device);
        return OUT_OF_RESOURCES;
    }

    IoAttachDevice(device, baseDeviceObject);

    info->hcd.hcdData = info;

    status = UhciConfigureController(baseDeviceObject, device, info);
    if(OK != status)
    {
        LOG(SYSLOG_ERROR, "Failed to configure UHCI controller, error 0x%X", (unsigned int)status);
        return status;
    }

    memset(&interface, 0, sizeof(interface));
    strcpy(interface.magic, USB_HC_IF_MAGIC);
    interface.version = USB_VERSION_1;
    interface.hcd = &info->hcd;
    interface.StartHostController = UhciStartHostController;
    interface.StopHostController = UhciStopHostController;
    interface.ResetHostController = UhciResetHostController;
    interface.GetRootHubData = UhciGetRootHubData;
    interface.GetRhPortStatus = UhciGetPortStatus;
    interface.SetEnableRhPort = UhciEnablePort;
    interface.ClearRhPortConnectChange = UhciClearPortConnectChange;
    interface.ClearRhPortEnableChange = UhciClearPortEnableChange;

    status = UhciHighLevelDriver->init(UhciHighLevelDriver, driverObject, &interface);
    if(OK != status)
    {
        LOG(SYSLOG_ERROR, "Failed to initialize high level driver, error 0x%X", (unsigned int)status);
    }
    return status;
}

/**
 * @brief Main driver entry routine, called only once when the driver is loaded to the memory
*/
STATUS DRIVER_ENTRY(struct ExDriverObject *driverObject, const char *dbPath)
{
    STATUS status = OK;
    struct ExDbHandle *db = nullptr;
    const char *usbDriver = nullptr;
    struct ExDriverObjectList *drv = nullptr;
    size_t drvCount = 0;

    driverObject->dispatch = UhciDispatch;
    driverObject->addDevice = UhciAddDevice;
    UhciLoggingInit();

    status = ExDbOpen(dbPath, &db);
    if(OK != status)
        goto leave;
    
    status = ExDbGetNextString(db, "HighLevelDriver", &usbDriver);
    if(OK != status)
        goto leave;
        
    status = ExLoadKernelDriversByName(usbDriver, &drv, &drvCount);
    if(OK != status)
        goto leave;

    if(1 != drvCount)
    {
        LOG(SYSLOG_ERROR, "Cannot support more than 1 high level driver");
        status = NOT_SUPPORTED;
        goto leave;
    }

    if(nullptr == drv->thisDriver->init)
    {
        LOG(SYSLOG_ERROR, "High level driver does not have an init routine");
        status = NOT_SUPPORTED;
        goto leave;
    }
    else
    {
        UhciHighLevelDriver = drv->thisDriver;
    }

leave:
    if(nullptr != db)
        ExDbClose(db);
    if(OK != status)
        LOG(SYSLOG_ERROR, "Driver startup failed with status 0x%X", (unsigned int)status);
    while(nullptr != drv)
    {
        void *old = drv;
        drv = drv->next;
        MmFreeKernelHeap(old);
    }
    return status;
}

