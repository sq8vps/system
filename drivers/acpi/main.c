#include "acpica/include/acpi.h"
#include "init.h"
#include "kernel.h"
#include "logging.h"
#include "device.h"


static struct IoRpQueue *rpQueue = NULL;

static void AcpiProcessRp(struct IoRp *rp)
{
    switch(rp->code)
    {
        case IO_RP_ENUMERATE:
            if(ACPI_FAILURE(DriverEnumerate(rp->device->driverObject, rp->device)))
                rp->status  = IO_RP_PROCESSING_FAILED;
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
            rp->status = IO_RP_CODE_UNKNOWN;
            break;
    }
    IoFinalizeRp(rp);
}

static STATUS AcpiDispatch(struct IoRp *rp)
{
    return IoStartRp(rpQueue, rp, NULL);
}

static STATUS AcpiInit(struct ExDriverObject *driverObject)
{
    STATUS ret = OK;
    if(OK != (ret = IoCreateRpQueue(AcpiProcessRp, &rpQueue)))
        return ret;

    if(AE_OK != AcInitialize())
        return EXEC_DRIVER_INIT_FAILED;
    
    return OK;
} 

static STATUS AcpiAddDevice(struct ExDriverObject *driverObject, struct IoDeviceObject *baseDeviceObject)
{
    //should never be called, there are no MDOs for ACPI
    return OK;
}

STATUS DRIVER_ENTRY(struct ExDriverObject *driverObject)
{
    driverObject->init = AcpiInit;
    driverObject->dispatch = AcpiDispatch;
    driverObject->addDevice = AcpiAddDevice;
    AcpiLoggingInit();
    return OK;
}

