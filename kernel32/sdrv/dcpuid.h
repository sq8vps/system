#ifndef KERNEL_CPUID_H_
#define KERNEL_CPUID_H_

/**
 * @file dcpuid.h
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
INTERNAL bool CpuidInit(void);

/**
 * @brief Check if CPUID instruction is available
 * @return True if available, false if not
*/
bool CpuidCheckIfAvailable(void);

EXPORT
/**
 * @brief Get CPU vendor string (13 characters incl. NULL terminator)
 * @param *dst Destination buffer
*/
EXTERN void CpuidGetVendorString(char *dst);

/**
 * @brief Check if APIC is available
 * @return True if available, false if not
*/
bool CpuidCheckIfApicAvailable(void);

/**
 * @brief Check if TSC is available
 * @return True if available, false if not
*/
bool CpuidCheckIfTscAvailable(void);

/**
 * @brief Check if MSR is available
 * @return True if available, false if not
*/
bool CpuidCheckIfMsrAvailable(void);

/**
 * @brief Check if TSC-Deadline APIC Timer mode is available
 * @return True if available, false if not
*/
bool CpuidCheckIfTscDeadlineAvailable(void);

/**
 * @brief Check if TSC clock is invariant across all power states
 * @return True if invariant, false if no invariance guaranteed
*/
bool CpuidCheckIfTscInvariant(void);

/**
 * @brief Check if FPU (x87) instructions are available
 * @return True if available, false if not
*/
bool CpuidCheckIfFpuAvailable(void);

/**
 * @brief Check if MMX instructions are available
 * @return True if available, false if not
*/
bool CpuidCheckIfMmxAvailable(void);

/**
 * @brief Check if SSE instructions are available
 * @return True if available, false if not
*/
bool CpuidCheckIfSseAvailable(void);

/**
 * @}
*/

#endif