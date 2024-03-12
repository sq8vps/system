#include "hal.h"
#include "sdrv/fpu.h"
#include "sdrv/sse.h"
#include "sdrv/dcpuid.h"
#include "ke/task/task.h"

static bool HalSseAvailable = false;

STATUS HalInitMath(void)
{
    if(!CpuidCheckIfFpuAvailable())
        return SYSTEM_INCOMPATIBLE;
    
    if(CpuidCheckIfSseAvailable())
    {
        HalSseAvailable = true;
        FpuInit();
        SseInit();
        return OK;
    }
    else
    {
        FpuInit();
        return OK;
    }
}

void *HalCreateMathStateBuffer(void)
{
    if(HalSseAvailable)
        return SseCreateStateBuffer();
    else
        return FpuCreateStateBuffer();
}

void HalStoreMathState(struct KeTaskControlBlock *tcb)
{
    if(HalSseAvailable)
        SseStore(tcb->mathState);
    else
        FpuStore(tcb->mathState);
}

void HalRestoreMathState(struct KeTaskControlBlock *tcb)
{
    if(HalSseAvailable)
        SseRestore(tcb->mathState);
    else
        FpuRestore(tcb->mathState);
}