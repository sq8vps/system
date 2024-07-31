#ifndef KERNEL_MSR_H_
#define KERNEL_MSR_H_

#include <stdint.h>
#include <stdbool.h>

#define MSR_IA32_TSC_DEADLINE 0x6E0

/**
 * @brief Initialize MSR module
 * @return True if MSR available, false if not
*/
bool MsrInit(void);

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

#endif