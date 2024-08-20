#include "cpu.h"

static struct HalCpu HalCpuList[MAX_CPU_COUNT];
static uint32_t HalCpuCount = 0;

STATUS HalRegisterCpu(struct HalCpuExtensions *extensions, bool usable)
{
    if(HalCpuCount >= MAX_CPU_COUNT)
        return OUT_OF_RESOURCES;
    HalCpuList[HalCpuCount].number = HalCpuCount;
    HalCpuList[HalCpuCount].usable = usable;
    if(NULL != extensions)
        HalCpuList[HalCpuCount].extensions = *extensions;
    HalCpuCount++;
    return OK;
}

struct HalCpu* HalGetCpuEntry(uint32_t id)
{
    if(id >= HalCpuCount)
        return NULL;
    
    return &HalCpuList[id];
}

uint32_t HalGetCpuCount(void)
{
    return HalCpuCount;
}