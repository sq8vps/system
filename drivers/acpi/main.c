#include "acpica/include/acpi.h"
#include "init.h"
#include "logging.h"
#include "device.h"
#include "io/dev/dev.h"
#include "io/dev/rp.h"
#include "ex/kdrv/kdrv.h"
#include "defines.h"


static struct IoRpQueue *rpQueue = NULL;

static void AcpiProcessRp(struct IoRp *rp)
{
    switch(rp->code)
    {
        case IO_RP_ENUMERATE:
            if(ACPI_FAILURE(DriverEnumerate(rp->device->driverObject, rp->device)))
                rp->status = NOT_SUPPORTED;
            else
                rp->status = OK;
            break;
        case IO_RP_GET_DEVICE_LOCATION:
            rp->status = AcpiGetDeviceLocation(rp);
            break;
        case IO_RP_GET_DEVICE_RESOURCES:
            rp->status = AcpiGetDeviceResources(rp);
            break;
        case IO_RP_GET_DEVICE_ID:
            rp->status = AcpiGetDeviceId(rp);
            break;
        default:
            rp->status = NOT_IMPLEMENTED;
            break;
    }
    IoFinalizeRp(rp);
}

static STATUS AcpiDispatch(struct IoRp *rp)
{
    return IoStartRp(rpQueue, rp, NULL);
}

STATUS AcpiMpAnalyze(void);

static STATUS AcpiAddDevice(struct ExDriverObject *driverObject, struct IoDeviceObject *baseDeviceObject)
{
    UNUSED(driverObject);
    UNUSED(baseDeviceObject);
    //should never be called, there are no MDOs for ACPI
    return DEVICE_NOT_AVAILABLE;
}

STATUS DRIVER_ENTRY(struct ExDriverObject *driverObject, const char *dbPath)
{
    UNUSED(dbPath);
    STATUS status = OK;
    driverObject->dispatch = AcpiDispatch;
    driverObject->addDevice = AcpiAddDevice;
    AcpiLoggingInit();
    status = IoCreateRpQueue(AcpiProcessRp, &rpQueue);
    if(OK != status)
        return status;

    AcpiMpAnalyze();

    if(AE_OK != AcInitialize())
        return DEVICE_NOT_AVAILABLE;
    
    return OK;
}

