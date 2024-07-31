#include "init.h"
#include "kernel.h"

#define _AcpiModuleName "Init"

static void NotifyHandler (
    ACPI_HANDLE                 Device,
    UINT32                      Value,
    void                        *Context)
{

    ACPI_INFO(("Received a notify 0x%x (device %p, context %p)", Value, Device, Context));
}

static ACPI_STATUS InstallHandlers (void)
{
    ACPI_STATUS             Status;


    /* Install global notify handler */

    Status = AcpiInstallNotifyHandler (ACPI_ROOT_OBJECT, ACPI_SYSTEM_NOTIFY,
                                        NotifyHandler, NULL);
    if (ACPI_FAILURE (Status))
    {
        ACPI_EXCEPTION ((AE_INFO, Status, "While installing Notify handler"));
        return (Status);
    }

    return (AE_OK);
}

static ACPI_STATUS acpicaInit(void)
{
    ACPI_STATUS             Status;

    /* Initialize the ACPICA subsystem */

    Status = AcpiInitializeSubsystem ();
    if (ACPI_FAILURE (Status))
    {
        ACPI_EXCEPTION ((AE_INFO, Status, "While initializing ACPICA"));
        return (Status);
    }

    /* Initialize the ACPICA Table Manager and get all ACPI tables */

    Status = AcpiInitializeTables (NULL, 0, FALSE);
    if (ACPI_FAILURE (Status))
    {
        ACPI_EXCEPTION ((AE_INFO, Status, "While initializing Table Manager"));
        return (Status);
    }

    /* Create the ACPI namespace from ACPI tables */

    Status = AcpiLoadTables ();
    if (ACPI_FAILURE (Status))
    {
        ACPI_EXCEPTION ((AE_INFO, Status, "While loading ACPI tables"));
        return (Status);
    }

    /* Install local handlers */

    Status = InstallHandlers ();
    if (ACPI_FAILURE (Status))
    {
        ACPI_EXCEPTION ((AE_INFO, Status, "While installing handlers"));
        return (Status);
    }

    /* Initialize the ACPI hardware */

    Status = AcpiEnableSubsystem (ACPI_FULL_INITIALIZATION);
    if (ACPI_FAILURE (Status))
    {
        ACPI_EXCEPTION ((AE_INFO, Status, "While enabling ACPICA"));
        return (Status);
    }

    /* Complete the ACPI namespace object initialization */

    Status = AcpiInitializeObjects (ACPI_FULL_INITIALIZATION);
    if (ACPI_FAILURE (Status))
    {
        ACPI_EXCEPTION ((AE_INFO, Status, "While initializing ACPICA objects"));
        return (Status);
    }

    return (AE_OK);
}

static void GlobalEventHandler(UINT32 EventType, ACPI_HANDLE Device,
		UINT32 EventNumber, void *Context) {
}

static ACPI_STATUS SetPicMode(bool enableIoApic)
{
    ACPI_STATUS             Status;
    ACPI_OBJECT_LIST        ArgList;
    ACPI_OBJECT             Arg[1];
    ACPI_BUFFER             ReturnValue;

    ArgList.Count = 1;
    ArgList.Pointer = Arg;

    Arg[0].Type = ACPI_TYPE_INTEGER;
    Arg[0].Integer.Value = enableIoApic ? 1 : 0;

    ReturnValue.Pointer = NULL;
    ReturnValue.Length = ACPI_ALLOCATE_BUFFER;

    Status = AcpiEvaluateObject (NULL, "\\_PIC", &ArgList, &ReturnValue);
	ACPI_FREE(ReturnValue.Pointer);
	if (Status == AE_NOT_FOUND)
	{
		return AE_OK;
	}
    if (ACPI_FAILURE (Status))
    {
        ACPI_EXCEPTION ((AE_INFO, Status, "While executing _PIC"));
        return Status;
    }

    return AE_OK;
}

ACPI_STATUS AcInitialize(void)
{
    ACPI_STATUS Status;

    Status = acpicaInit();
    if(ACPI_FAILURE(Status))
        return Status;
    
    Status = SetPicMode(I686IsIoApicUsed());

    AcpiWriteBitRegister(ACPI_BITREG_SCI_ENABLE, 1);
	AcpiInstallGlobalEventHandler(GlobalEventHandler, NULL);


    return (AE_OK);
}