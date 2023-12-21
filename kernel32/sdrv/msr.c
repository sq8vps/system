#include "msr.h"
#include "dcpuid.h"

static bool msrAvailable = false;

bool HalMsrInit(void)
{
    if(CpuidCheckIfMsrAvailable())
    {
        msrAvailable = true;
        return true;
    }
    return false;
}

uint64_t HalMsrGet(uint32_t msr)
{
    if(!msrAvailable)
        return 0;

    uint32_t low, high;
    ASM("rdmsr" : "=a" (low), "=d" (high) : "c" (msr));
    return (((uint64_t)high) << 32) | ((uint64_t)low);
}
 
void HalMsrSet(uint32_t msr, uint64_t val)
{
    if(!msrAvailable)
        return;

   ASM("wrmsr" : : "a" ((uint32_t)(val & 0xFFFFFFFF)), "d" ((uint32_t)(val >> 32)), "c" (msr));
}