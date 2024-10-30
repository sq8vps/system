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

void HalDestroyMathStateBuffer(const void *math)
{
    if(HalSseAvailable)
        return SseDestroyStateBuffer(math);
    else
        return FpuDestroyStateBuffer(math);
}

__attribute__ ((fastcall))
void HalStoreMathState(struct KeTaskControlBlock *tcb)
{
    if(HalSseAvailable)
        SseStore(tcb->data.fpu);
    else
        FpuStore(tcb->data.fpu);
}

__attribute__ ((fastcall))
void HalRestoreMathState(struct KeTaskControlBlock *tcb)
{
    if(HalSseAvailable)
        SseRestore(tcb->data.fpu);
    else
        FpuRestore(tcb->data.fpu);
}

#endif