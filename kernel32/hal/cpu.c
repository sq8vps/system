#include "cpu.h"
#include "sdrv/mp.h"
#include "sdrv/dcpuid.h"
#include "sdrv/lapic.h"
#include "sdrv/acpi.h"
#include "common.h"
#include "io/disp/print.h"

struct HalCpuConfig HalCpuConfigTable[MAX_CPU_COUNT];
uint8_t HalCpuCount = 0;

enum HalCpuConfigMethod cpuConfigMethod = CPU_METHOD_NONE;
static uint32_t lapicAddress = 0;

STATUS HalInitCpu(void)
{
    STATUS ret = OK;

    if(OK == AcpiInit(&lapicAddress)) //try to initialize ACPI
    {
        LOG("ACPI-compliant system\n");
        cpuConfigMethod = CPU_METHOD_ACPI;
        return OK;
    }
    else if(OK == (ret = MpInit(&lapicAddress))) //fallback to MP if ACPI not available
    {
        LOG("MPS-compliant system\n");
        cpuConfigMethod = CPU_METHOD_MP;
        return OK;
    }
    IoPrint("System is not ACPI nor MP compliant!\n");
    
    return ret;
}

uint32_t HalGetLapicAddress(void)
{
    return lapicAddress;
}

#ifdef DEBUG
void HalListCpu(void)
{
    PRINT("Local APIC is at 0x%X\n", (unsigned int)lapicAddress);
    for(uint8_t i = 0; i < HalCpuCount; i++)
    {
        PRINT("CPU %d: APIC ID=0x%X", (int)i, (unsigned int)HalCpuConfigTable[i].apicId);
        if(HalCpuConfigTable[i].boot)
            PRINT(", bootstrap processor");
        if(HalCpuConfigTable[i].usable)
            PRINT(", usable");
        PRINT("\n");
    }
}
#endif