#include "acpica/include/acpi.h"
#include "init.h"
#include "kernel.h"
#include "logging.h"
#include "device.h"

static char driverName[] = "Generic ACPI system driver";
static char driverVendor[] = "Standard drivers";
static char driverVersion[] = "1.0.0";

static struct IoRpQueue *rpQueue = NULL;

static void DriverProcessRp(struct IoDriverRp *rp)
{
    STATUS status = OK;
    switch(rp->code)
    {
        case IO_RP_ENUMERATE:
            if(ACPI_FAILURE(DriverEnumerate(rp->device->driverObject)))
            {
                status = IO_RP_PROCESSING_FAILED;
            }
            break;
        case IO_RP_GET_BUS_CONFIGURATION:
            struct BusSubDeviceInfo *busConfig;
            if(ACPI_FAILURE(DriverGetBusInfoForDevice(rp->payload.busConfiguration.device, &busConfig)))
            {
                status = IO_RP_PROCESSING_FAILED;
            }
            if((NULL != busConfig) && (IO_BUS_TYPE_PCI == busConfig->type))
            {
                rp->payload.busConfiguration.pci.bus = busConfig->id.pci.bus;
                rp->payload.busConfiguration.pci.device = busConfig->id.pci.device;
                rp->payload.busConfiguration.pci.function = busConfig->id.pci.function;
            }
            else
                status = IO_RP_PROCESSING_FAILED;
            break;
        case IO_RP_GET_MMIO_MAPPING:
            
            break;
        default:
                status = IO_RP_CODE_UNKNOWN;
            break;
    }
    rp->status = status;
    IoFinalizeRp(rp);
}

static STATUS DriverDispatch(struct IoDriverRp *rp)
{
    // switch(rp->code)
    // {
    //     case IO_RP_ENUMERATE:
    //         return IoStartRp(rpQueue, rp, NULL);
    //         break;
    //     case IO_RP_GET_BUS_CONFIGURATION:
    //         return IoStartRp(rpQueue, rp, NULL);
    //         break;
    //     default:
    //         return IoSendRpDown(rp->device, rp);
    //         break;
    // }
    DriverProcessRp(rp);
    return OK;
}

static STATUS DriverInit(struct ExDriverObject *driverObject)
{
    STATUS ret = OK;
    if(OK != (ret = IoCreateRpQueue(DriverProcessRp, &rpQueue)))
        return ret;

    if(AE_OK != AcInitialize())
        return EXEC_DRIVER_INIT_FAILED;
    
    return OK;
} 

static STATUS DriverAddDevice(struct ExDriverObject *driverObject, struct IoSubDeviceObject *baseDeviceObject)
{
    return OK;
}

STATUS DRIVER_ENTRY(struct ExDriverObject *driverObject)
{
    driverObject->init = DriverInit;
    driverObject->dispatch = DriverDispatch;
    driverObject->addDevice = DriverAddDevice;
    driverObject->name = driverName;
    driverObject->vendor = driverVendor;
    driverObject->version = driverVersion;
    AcpiLoggingInit();
    return OK;
}

