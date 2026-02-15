/**
 * @file msr.h
 * @brief Model-specific register manipulation
 * @ingroup i686
 */

#ifndef KERNEL_MSR_H_
#define KERNEL_MSR_H_

#include <stdint.h>
#include <stdbool.h>
#include "defines.h"

/**
 * @addtogroup i686
 * @{
 */

#define MSR_IA32_TSC_DEADLINE 0x6E0 /**< TSC deadline value register */
#define MSR_IA32_APIC_BASE 0x1B /**< Local APIC base register */
#define MSR_IA32_SYSENTER_CS 0x174 /**< CS for sysenter register */
#define MSR_IA32_SYSENTER_ESP 0x175 /**< ESP for sysenter register */
#define MSR_IA32_SYSENTER_EIP 0x176 /**< EIP for sysenter register */

#define MSR_IA32_APIC_BASE_ENABLE_MASK 0x800 /**< APIC enable mask */
#define MSR_IA32_APIC_BASE_BSP_MASK 0x100 /**< BSP mask  */

/**
 * @brief Initialize MSR module
 * @return True if MSR available, false if not
 * @kinternal
*/
INTERNAL bool MsrInit(void);

DRIVER_API

/**
 * @brief Get Model Specific Register value
 * @param msr MSR number
 * @return MSR value
*/
uint64_t MsrGet(uint32_t msr);

/**
 * @brief Set Model Specific Register
 * @param msr MSR number
 * @param val Value to set
*/
void MsrSet(uint32_t msr, uint64_t val);

END_DRIVER_API

/**
 * @}
 */

#endif