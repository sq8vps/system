#if defined(__i686__) || defined(__amd64__)

#include "hal/hal.h"
#include "msr.h"
#include "dcpuid.h"
#include "ke/core/panic.h"

STATUS HalInitializeHardware(void)
{
    if(!CpuidCheckIfAvailable())
        FAIL_BOOT("CPUID not available");
    
    CpuidInit();

    if(!CpuidCheckIfFpuAvailable())
        FAIL_BOOT("FPU not available");
    
    MsrInit();
    return OK;
}

#endif