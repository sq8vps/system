//This header file is generated automatically
#ifndef EXPORTED___API__HAL_HAL_H_
#define EXPORTED___API__HAL_HAL_H_

#ifdef __cplusplus
extern "C" 
{
#endif

#include <stdint.h>
#include "defines.h"
#include <stdbool.h>

/**
 * @brief Check if buffer is accessible in user mode
 * @param *buffer Buffer to be validated
 * @param size Buffer size
 * @return True if valid, false if invalid
 */
bool HalValidateUserBuffer(const void *buffer, uintptr_t size);


#ifdef __cplusplus
}
#endif

#endif