#if defined(__i686__) || defined(__amd64__)

#include "hal/time.h"
#include "msr.h"
#include "fpu.h"
#include "sse.h"
#include "dcpuid.h"
#include "ke/task/task.h"
#include <stdbool.h>

static bool HalSseAvailable = false;

STATUS I686InitMath(void)
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

#endif