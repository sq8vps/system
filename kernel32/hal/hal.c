#include "hal.h"
#include "cpu.h"
#include "interrupt.h"
#include "sdrv/dcpuid.h"
#include "sdrv/tsc.h"
#include "sdrv/msr.h"
#include "time.h"

static STATUS verifyRequiredCapatibities(void)
{
    uint8_t available = 1;

    //this function checks for capabilities required to run this OS
    available &= CpuidCheckIfFpuAvailable();
    available &= CpuidCheckIfMmxAvailable();
    available &= CpuidCheckIfSseAvailable();
    available &= CpuidCheckIfTscAvailable();

    if(!available)
        return SYSTEM_INCOMPATIBLE;
    
    return OK;
}

STATUS HalInit(void)
{
    if(!CpuidCheckIfAvailable())
        return SYSTEM_INCOMPATIBLE;
    
    CpuidInit();

    STATUS ret = OK;
    if(OK != (ret = verifyRequiredCapatibities()))
        return ret;
    
    HalMsrInit();

    if(OK != (ret = HalInitCpu()))
        return ret;
    
    if(OK != (ret = HalInitInterruptController()))
        return ret;

    TscInit();
    if(OK != HalInitTimeController())
        return ret;

    return OK;
}

