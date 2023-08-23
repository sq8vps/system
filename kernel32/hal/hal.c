#include "hal.h"
#include "cpu.h"
#include "interrupt.h"
#include "sdrv/dcpuid.h"
#include "sdrv/tsc.h"
#include "sdrv/msr.h"
#include "time.h"


STATUS HalInit(void)
{
    CpuidInit();
    HalMsrInit();

    STATUS ret = OK;
    if(OK != (ret = HalInitCpu()))
        return ret;
    
    if(OK != (ret = HalInitInterruptController()))
        return ret;

    TscInit();
    if(OK != HalInitTimeController())
        return ret;

    return OK;
}

