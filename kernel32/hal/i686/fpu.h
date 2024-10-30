#ifndef KERNEL_FPU_H_
#define KERNEL_FPU_H_

#include <stdint.h>
#include "defines.h"

/**
 * @brief Initialize x87 FPU
 * @attention This function assumes that the 80387+ coprocessor is available
*/
INTERNAL void FpuInit(void);

/**
 * @brief Handle x87 FPU exceptions
*/
INTERNAL void FpuHandleException(void);

/**
 * @brief Create buffer for FPU state storage
 * @return Buffer pointer or NULL on failure
*/
INTERNAL void *FpuCreateStateBuffer(void);

/**
 * @brief Destroy FPU state storage buffer
 * @param *math Buffer pointer
 */
INTERNAL void FpuDestroyStateBuffer(const void *math);

/**
 * @brief Store x87 and MMX state
 * @param *buffer Buffer to store the FPU state
 * @warning If SSE is available, then \a SseStore() must be used
*/
INTERNAL void FpuStore(void *buffer);

/**
 * @brief Retore x87 and MMX state
 * @param *buffer Buffer to restore the FPU state from
 * @warning If SSE is available, then \a SseRestore() must be used
*/
INTERNAL void FpuRestore(void *buffer);

#endif