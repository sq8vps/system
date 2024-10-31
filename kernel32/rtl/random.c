#include "config.h"
#include "hal/arch.h"
#include "hal/cpu.h"
#include "hal/time.h"

#ifndef SMP
static uint32_t RtlRandomNext = 1;
#else
static uint32_t RtlRandomNext[MAX_CPU_COUNT] = {[0 ... MAX_CPU_COUNT - 1] = 1};
#endif

int32_t RtlRandom(int32_t min, int32_t max)
{
#ifndef SMP
    RtlRandomNext = RtlRandomNext * 1103515245 + 12345;
    return ((RtlRandomNext / ((max - min + 1) * 2)) % (max - min + 1)) + min;
#else
    uint16_t cpu = HalGetCurrentCpu();
    RtlRandomNext[cpu] = RtlRandomNext[cpu] * 1103515245 + 12345;
    return ((RtlRandomNext[cpu] / ((max - min + 1) * 2)) % (max - min + 1)) + min;
#endif
}

void RtlInitializeRandom(void)
{
#ifndef SMP
    RtlRandomNext = HalGetTimestamp();
#else
    for(uint16_t i = 0; i < HalGetCpuCount(); ++i)
        RtlRandomNext[i] = HalGetTimestamp();
#endif
}