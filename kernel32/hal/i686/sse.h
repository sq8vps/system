#ifndef KERNEL_SSE_H_
#define KERNEL_SSE_H_

#include "defines.h"

/**
 * @brief Initialize SSE
 * @attention This function assumes that the SSE instruction set is available
 * @warning This function does not initialize x87 FPU
*/
INTERNAL void SseInit(void);


/**
 * @brief Create buffer for x87/MMX/SSE state storage
 * @return Buffer pointer or NULL on failure
*/
INTERNAL void *SseCreateStateBuffer(void);

/**
 * @brief Store x87/MMX/SSE state
 * @param *buffer Buffer to store the SSE state
*/
INTERNAL void SseStore(void *buffer);

/**
 * @brief Restore x87/MMX/SSE state
 * @param *buffer Buffer to restore the SSE state from
*/
INTERNAL void SseRestore(void *buffer);

#endif