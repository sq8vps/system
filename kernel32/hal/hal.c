#include "hal.h"
#include "cpu.h"
#include "interrupt.h"
#include "sdrv/cpuid.h"




STATUS HalInit(void)
{
    CpuidInit();

    STATUS ret = OK;
    if(OK != (ret = HalInitCpu()))
        return ret;
    
    if(OK != (ret = HalInitInterruptController()))
        return ret;

    return OK;
}

