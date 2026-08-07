#include "logging.h"
#include "io/dev/rp.h"
#include "io/dev/dev.h"
#include "ex/kdrv/kdrv.h"
#include "mm/heap.h"
#include "rtl/string.h"
#include "hc.h"
#include "ex/db/db.h"
#include "if.h"
#include "xfer.h"

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
 * @brief Dummy, this should be handled by the high level driver
*/
static STATUS UhciAddDevice(struct ExDriverObject *driverObject, struct IoDeviceObject *baseDeviceObject)
{
    return NOT_SUPPORTED;
}

/**
 * @brief Main driver entry routine, called only once when the driver is loaded to the memory
*/
STATUS DRIVER_ENTRY(struct ExDriverObject *driverObject, const char *dbPath)
{
    STATUS status = OK;
    struct ExDbHandle *db = nullptr;
    char *usbDriver = nullptr;
    struct ExDriverObjectList *drv = nullptr;
    size_t drvCount = 0;
    struct UsbHcInterface interface;

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

    memset(&interface, 0, sizeof(interface));
    strcpy(interface.magic, USB_HC_IF_MAGIC);
    interface.version = USB_VERSION_1;
    interface.transferContextSize = sizeof(struct UhciTransfer);
    interface.InitializeHostController = UhciInitializeHostController;
    interface.StartHostController = UhciStartHostController;
    interface.StopHostController = UhciStopHostController;
    interface.ResetHostController = UhciResetHostController;
    interface.GetRootHubData = UhciGetRootHubData;
    interface.GetRhPortStatus = UhciGetPortStatus;
    interface.SetEnableRhPort = UhciEnablePort;
    interface.ClearRhPortConnectChange = UhciClearPortConnectChange;
    interface.ClearRhPortEnableChange = UhciClearPortEnableChange;

    status = drv->thisDriver->init(drv->thisDriver, driverObject, &interface);
    if(OK != status)
    {
        LOG(SYSLOG_ERROR, "Failed to initialize high level driver, error 0x%X", (unsigned int)status);
        goto leave;
    }

    if((nullptr == drv->thisDriver->dispatch) || (nullptr == drv->thisDriver->addDevice))
    {
        LOG(SYSLOG_ERROR, "High level driver does not have a dispatch or add device routine");
        status = NOT_SUPPORTED;
        goto leave;
    }

    driverObject->dispatch = drv->thisDriver->dispatch;
    driverObject->addDevice = drv->thisDriver->addDevice;

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

