#ifndef KERNEL32_PANIC_H_
#define KERNEL32_PANIC_H_

/**
 * @file panic.h
 * @brief Kernel panic/emergency shutdown kernel routines
 * 
 * Provides routines for shutting down system in a controlled manner
 * when there is an unrecoverable failure.
 * 
 * @defgroup ke Kernel core routines
*/

#include <stdint.h>
#include "../defines.h"

/**
 * @defgroup panic Kernel panic routines
 * @ingroup ke
 * @{
*/

/**
 * @brief Emergency system shutdown routine - kernel panic
 * @param code Error code
 * @attention This function never returns
*/
EXPORT NORETURN
void KePanic(uintptr_t code);

/**
 * @brief Emergency system shutdown routine - kernel panic
 * @param code Error code
 * @param arg1 Argument 1
 * @param arg2 Argument 2
 * @param arg3 Argument 3
 * @param arg4 Argument 4
 * @attention This function never returns
*/
EXPORT NORETURN
void KePanicEx(uintptr_t code, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3, uintptr_t arg4);

/**
 * @}
*/

#endif