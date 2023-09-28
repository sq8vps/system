#include "acpica/include/acpi.h"
#include "kernel.h"

static char driverName[] = "ACPI root system driver";
static char driverVendor[] = "Standard drivers";
static char driverVersion[] = "1.0.0";

static ACPI_STATUS initializeAcpi(void)
{
    ACPI_STATUS Status;

    Status = AcpiInitializeSubsystem();
    if (ACPI_FAILURE(Status))
    {
        return (Status);
    }
 
    Status = AcpiInitializeTables(NULL, 16, FALSE);
    if (ACPI_FAILURE(Status))
    {
        return (Status);
    }

    Status = AcpiLoadTables();
    if (ACPI_FAILURE(Status))
    {
        return (Status);
    }

    Status = AcpiEnableSubsystem(ACPI_FULL_INITIALIZATION);
    if (ACPI_FAILURE(Status))
    {
        return (Status);
    }
  
    Status = AcpiInitializeObjects(ACPI_FULL_INITIALIZATION);
    if (ACPI_FAILURE(Status))
    {
        return (Status);
    }
    return (AE_OK);
}

STATUS init(struct ExDriverObject *driverObject)
{
    if(AE_OK != initializeAcpi())
        return EXEC_DRIVER_INIT_FAILED;
    
    return OK;
} 

STATUS dispatch(struct IoDriverIRP *irp)
{
    return OK;
}

STATUS DRIVER_ENTRY(struct ExDriverObject *driverObject)
{
    driverObject->driverInit = init;
    driverObject->driverDispatchIrp = dispatch;
    driverObject->name = driverName;
    driverObject->vendor = driverVendor;
    driverObject->version = driverVersion;
    return OK;
}

