#ifndef KERNEL_CPUID_H_
#define KERNEL_CPUID_H_

/**
 * @file dcpuid.h
 * @brief CPUID module
 * 
 * A system core driver providing access to CPUID information from the CPU.
 * 
 * @ingroup i686 amd64
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

EXPORT_API

/**
 * @brief Check if CPUID instruction is available
 * @return True if available, false if not
*/
bool CpuidCheckIfAvailable(void);


/**
 * @brief Get CPU vendor string (13 characters incl. NULL terminator)
 * @param *dst Destination buffer
*/
void CpuidGetVendorString(char *dst);

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
 * @brief Get current CPU local APIC ID
 * @return Local APIC ID
 */
uint8_t CpuidGetApicId(void);

END_EXPORT_API

/**
 * @}
*/

#endif