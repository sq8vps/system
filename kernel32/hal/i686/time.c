#if defined(__i686__) || defined(__amd64__)

#include "hal/time.h"
#include "dcpuid.h"
#include "tsc.h"
#include "lapic.h"
#include <stdbool.h>

static bool HalUseTsc = false;

STATUS I686InitTimeController(void)
{
    if(CpuidCheckIfTscAvailable() && CpuidCheckIfTscInvariant())
    {
        HalUseTsc = true;
        TscInit();
    }
    
    return OK;
}

uint64_t HalGetTimestamp(void)
{
    if(HalUseTsc)
        return TscGet();
    else
        return ApicGetTimestamp();
}

uint64_t HalGetTimestampMicros(void)
{
    if(HalUseTsc)
        return TscGetMicros();
    else
        return ApicGetTimestampMicros();
}

uint64_t HalGetTimestampMillis(void)
{
    if(HalUseTsc)
        return TscGetMillis();
    else
        return ApicGetTimestampMillis();
}

STATUS HalConfigureSystemTimer(uint8_t vector)
{   
    return ApicConfigureSystemTimer(vector);
}

STATUS HalStartSystemTimer(uint64_t time)
{
    ApicStartSystemTimer(time);
    return OK;
}

#endif