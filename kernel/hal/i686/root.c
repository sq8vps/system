#if defined(__i686__) || defined(__amd64__)

#include "lapic.h"
#include "acpi.h"
#include "dcpuid.h"
#include "hal/hal.h"

#include "ke/core/panic.h"
#include "irq.h"

STATUS I686InitRoot(void)
{
    uintptr_t address; //lapic address

    I686SetDefaultIsaRemap();

    if(OK == AcpiInit(&address)) //try to initialize ACPI
    {
        HalSetRootDeviceId("ACPI");
    }
    else
        FAIL_BOOT("system is not ACPI compliant");
    
    if(!CpuidCheckIfApicAvailable())
        FAIL_BOOT("local APIC is not available")
    
    if(OK != ApicInit(address))
        FAIL_BOOT("local APIC initialization failed");
    
    return OK;
}

#endif