#if defined(__i686__) || defined(__amd64__)

#include "mp.h"
#include "lapic.h"
#include "acpi.h"
#include "dcpuid.h"
#include "hal/hal.h"
#include "common.h"
#include "ke/core/panic.h"
#include "irq.h"

STATUS I686InitRoot(void)
{
    STATUS status = OK;
    uintptr_t address; //lapic address

    I686SetDefaultIsaRemap();

    if(OK == AcpiInit(&address)) //try to initialize ACPI
    {
        LOG("ACPI-compliant system\n");
        HalSetRootDeviceId("ACPI");
    }
    else if(OK == (status = MpInit(&address))) //fallback to MP if ACPI not available
    {
        LOG("MPS-compliant system\n");
        HalSetRootDeviceId("MP");
    }
    else
        FAIL_BOOT("system is not ACPI nor MPS compliant");
    
    if(!CpuidCheckIfApicAvailable())
        FAIL_BOOT("local APIC is not available")
    
    if(OK != ApicInit(address))
        FAIL_BOOT("local APIC initialization failed");
    
    return OK;
}

#endif