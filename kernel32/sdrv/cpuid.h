#ifndef KERNEL_CPUID_H_
#define KERNEL_CPUID_H_

/**
 * @file cpuid.h
 * @brief CPUID moduke
 * 
 * A system core driver providing access to CPUID information from the CPU.
 * 
 * @ingroup sdrv
 * @defgroup cpuid CPUID routines
*/

#include <stdbool.h>
#include <stdint.h>
#include "defines.h"

/**
 * @addtogroup cpuid
 * @{
*/

/**
 * @brief Initialize CPUID module
 * @return True if CPUID available, false if not
*/
bool CpuidInit(void);

/**
 * @brief Check if CPUID instruction is available
 * @return True if available, false if not
*/
bool CpuidCheckIfAvailable(void);

/**
 * @brief Get CPU vendor string (13 characters incl. NULL terminator)
 * @param *dst Destination buffer
*/
EXPORT void CpuidGetVendorString(char *dst);

/**
 * @brief Check if APIC is available
 * @return True if available, false if not
*/
bool CpuidCheckIfApicAvailable(void);

/**
 * @brief Check if MSR is available
 * @return True if available, false if not
*/
bool CpuidCheckIfMsrAvailable(void);

/**
 * @}
*/

#endif