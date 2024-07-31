#if defined(__i686__) || defined(__amd64__)

#include "tsc.h"
#include "dcpuid.h"
#include <stdbool.h>
#include "defines.h"
#include "pit.h"
#include "common.h"

static bool tscAvailable = false;
static uint64_t frequency = 0;

STATUS TscInit(void)
{
    if(CpuidCheckIfAvailable())
    {
        if(CpuidCheckIfTscAvailable())
        {
            if(CpuidCheckIfTscInvariant())
            {
                LOG("Invariant TSC available\n");
                tscAvailable = true;
                TscCalibrate();
                return OK;
            }
        }
    }
    return DEVICE_NOT_AVAILABLE;
}

uint64_t TscGetRaw(void)
{
    if(tscAvailable)
    {
        uint32_t hi, lo;
        ASM("rdtsc" : : : "edx", "eax");
        ASM("mov %0,edx" : "=m"(hi) : : "memory");
        ASM("mov %0,eax" : "=m"(lo) : : "memory");
        return (((uint64_t)hi) << 32) | ((uint64_t)lo);
    }
    return 0;
}

STATUS TscCalibrate(void)
{
    if(!tscAvailable)
        return DEVICE_NOT_AVAILABLE;
        
    PitOneShotInit(10000); //10000us=10ms
    PitOneShotStart();
    uint64_t value = TscGetRaw();
    PitOneShotWait();
    value = TscGetRaw() - value;
    frequency = ((uint64_t)100) * value;
    return OK;
}

uint64_t TscGet(void)
{
    if(!tscAvailable)
        return 0;
    
    return (uint64_t)((1000000000. * (double)TscGetRaw()) / (double)frequency);
}

uint64_t TscGetMicros(void)
{
    if(!tscAvailable)
        return 0;
    
    return (uint64_t)((1000000. * (double)TscGetRaw()) / (double)frequency);
}

uint64_t TscGetMillis(void)
{
    if(!tscAvailable)
        return 0;
    
    return (uint64_t)((1000. * (double)TscGetRaw()) / (double)frequency);
}

uint64_t TscCalculateRaw(uint64_t time)
{
    return (uint64_t)((double)frequency * (double)time / 1000000000.f);
}

#endif