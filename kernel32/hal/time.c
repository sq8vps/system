#include "time.h"
#include "sdrv/tsc.h"
#include "sdrv/lapic.h"
#include <stdbool.h>
#include "sdrv/dcpuid.h"

static bool useTsc = false;

STATUS HalInitTimeController(void)
{
    if(CpuidCheckIfTscAvailable() && CpuidCheckIfTscInvariant())
    {
        useTsc = true;
        TscInit();
    }
    
    return OK;
}

uint64_t HalGetTimestamp(void)
{
    if(useTsc)
        return TscGet();
    else
        return ApicGetTimestamp();
}

uint64_t HalGetTimestampMicros(void)
{
    if(useTsc)
        return TscGetMicros();
    else
        return ApicGetTimestampMicros();
}

uint64_t HalGetTimestampMillis(void)
{
    if(useTsc)
        return TscGetMillis();
    else
        return ApicGetTimestampMillis();
}