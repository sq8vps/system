#include "cpu.h"

struct Cpu
{
    uint8_t lapic;
    bool usable;
    bool bootstrap;
};

static struct Cpu CpuList[MAX_CPU_COUNT];
static uint32_t CpuCount = 0;

void CpuAdd(uint8_t lapic, bool usable, bool bootstrap)
{
    if(CpuCount >= MAX_CPU_COUNT)
        return;
    
    CpuList[CpuCount].lapic = lapic;
    CpuList[CpuCount].usable = usable;
    CpuList[CpuCount].bootstrap = bootstrap;
    CpuCount++;
}