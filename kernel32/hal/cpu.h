#ifndef KERNEL_CPU_H_
#define KERNEL_CPU_H_

/**
 * @file cpu.h
 * @brief HAL CPU module
 * 
 * A HAL module providing routines for CPU detection and initialization.
 * 
 * @ingroup hal
 * @defgroup cpu HAL CPU module
*/

#include <stdint.h>
#include "defines.h"
#include <stdbool.h>

/**
 * @addtogroup cpu
 * @{
*/

enum HalCpuConfigMethod
{
    CPU_METHOD_NONE,
    CPU_METHOD_MP,
    CPU_METHOD_ACPI,
};

struct HalCpuConfig
{
    uint8_t apicId;
    uint8_t usable : 1;
    uint8_t boot : 1;
};

extern struct HalCpuConfig HalCpuConfigTable[MAX_CPU_COUNT];
extern uint8_t HalCpuCount;

/**
 * @brief Initialize CPU configuratzions (SMP, APIC etc.)
 * @return Status code
*/
STATUS HalInitCpu(void);

/**
 * @brief Get Local APIC physicall address
 * @return LAPIC physical address or 0 if not available
*/
uint32_t HalGetLapicAddress(void);

#ifdef DEBUG
void HalListCpu(void);
#endif

/**
 * @}
*/

#endif