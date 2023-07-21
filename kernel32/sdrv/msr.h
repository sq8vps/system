#ifndef KERNEL_MSR_H_
#define KERNEL_MSR_H_

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Initialize MSR module
 * @return True if MSR available, false if not
*/
bool HalMsrInit(void);

/**
 * @brief Get Model Specific Register value
 * @param msr MSR number
 * @return MSR value
*/
uint64_t HalGetMsr(uint32_t msr);

/**
 * @brief Set Model Specific Register
 * @param msr MSR number
 * @param val Value to set
*/
void HalSetMsr(uint32_t msr, uint64_t val);

#endif