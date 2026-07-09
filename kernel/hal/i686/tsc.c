#if defined(__i686__) || defined(__amd64__)

#include "tsc.h"
#include "dcpuid.h"
#include <stdbool.h>
#include "defines.h"
#include "pit.h"
#include "config.h"
#include "hal/time.h"
#include "ipi.h"
#include "rtl/string.h"

static volatile struct
{
    bool present; /**< TSC is present */
    int64_t offset; /**< Per-core TSC offset relative to BSP */
    bool invariant; /**< TSC is invariant */
} TscState[MAX_CPU_COUNT];
static bool TscAvailable = false;

static struct HalClockSource TscClockSource = 
{
    .name = "TSC",
    .rating = 500,
    .read = TscGetRaw,
    .context = NULL,
};

static int TscGetOffset(void *context)
{
    const int64_t offset = *((uint64_t*)context) - TscGetRaw(NULL);
    barrier();
    const uint32_t cpu = HalGetCurrentCpu();

    TscState[cpu].present = true;
    TscState[cpu].invariant = CpuidCheckIfTscInvariant();
    TscState[cpu].offset = offset;

    return 1;
}
//TODO: probably also check whether TSC is available on other APs...

STATUS TscInitForSmp(void)
{
    HalCpuBitmap cpus = HAL_CPU_ALL;
    int results[MAX_CPU_COUNT] = {0};

    uint64_t current = TscGetRaw(NULL);
    I686InvokeRemoteFunction(&cpus, TscGetOffset, &current, results);

    for(uint32_t i = 0; i < MAX_CPU_COUNT; i++)
    {
        if(results[i] && (!TscState[i].present || !TscState[i].invariant))
        {
            TscClockSource.rating = 0;
            break;
        }
    }

    return OK;
}

STATUS TscInit(void)
{
    RtlMemsetV(TscState, 0, sizeof(TscState));

    uint32_t cpu = HalGetCurrentCpu();

    TscState[cpu].present = true;

    if(CpuidCheckIfTscInvariant())
        TscState[cpu].invariant = true;
    else
        TscClockSource.rating = 0;

    TscAvailable = true;

    TscCalibrate();
    return HalRegisterClockSource(&TscClockSource);
}

uint64_t TscGetRaw(void *context)
{
    UNUSED(context);
    uint32_t hi, lo;
    ASM("rdtsc" : : : "edx", "eax");
    ASM("mov %0,edx" : "=m"(hi) : : "memory");
    ASM("mov %0,eax" : "=m"(lo) : : "memory");
    uint64_t timestamp = (((uint64_t)hi) << 32) | ((uint64_t)lo);
    return timestamp + TscState[HalGetCurrentCpu()].offset;
}

void TscUpdate(void)
{
    if(unlikely(!TscAvailable))
        return;
    HalUpdateClockSource(&TscClockSource);
}

static void TscMeasurementCallback(bool finished, void *context)
{
    uint64_t *value = (uint64_t*)context;
    if(!finished)
    {
       *value = TscGetRaw(NULL);
    }
    else
    {
        *value = TscGetRaw(NULL) - *value;
        TscClockSource.frequency = ((uint64_t)100) * *value;
    }
}

STATUS TscCalibrate(void)
{
    uint64_t value; 
    PitDoSingleShot(10000, TscMeasurementCallback, TscMeasurementCallback, &value);

    return OK;
}

uint64_t TscCalculateRaw(uint32_t time)
{
    return (TscClockSource.frequency * (uint64_t)time) / (uint64_t)1000000000;
}

#endif