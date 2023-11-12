#ifndef KERNEL_PANIC_H_
#define KERNEL_PANIC_H_

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
#include "defines.h"

/**
 * @defgroup panic Kernel panic routines
 * @ingroup ke
 * @{
*/

EXPORT
/**
 * @brief Main kernel panic error codes
*/
enum KernelPanicCode
{
    NON_MASKABLE_INTERRUPT,
    DIVISION_BY_ZERO,
    INVALID_OPCODE,
    DOUBLE_FAULT,
    GENERAL_PROTECTION_FAULT,
    BOOT_FAILURE,
    NO_EXECUTABLE_TASK,
    UNACQUIRED_MUTEX_RELEASED,
    BUSY_MUTEX_ACQUIRED,
    PAGE_FAULT,
    MACHINE_CHECK_FAULT,
    UNEXPECTED_FAULT,
    ACPI_FATAL_ERROR,
};

EXPORT
/**
 * @brief Emergency system shutdown routine - kernel panic
 * @param code Error code
 * @attention This function never returns
*/
EXTERN NORETURN void KePanic(uintptr_t code);

EXPORT
/**
 * @brief Emergency system shutdown routine - kernel panic
 * @param code Error code
 * @param arg1 Argument 1
 * @param arg2 Argument 2
 * @param arg3 Argument 3
 * @param arg4 Argument 4
 * @attention This function never returns
*/
EXTERN NORETURN void KePanicEx(uintptr_t code, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3, uintptr_t arg4);

EXPORT
/**
 * @brief Emergency system shutdown routine - kernel panic with explicitly provided instruction pointer
 * @param ip Failing instruction pointer
 * @param code Error code
 * @attention This function never returns
*/
EXTERN NORETURN void KePanicIP(uintptr_t ip, uintptr_t code);

EXPORT
/**
 * @brief Emergency system shutdown routine - kernel panic with explicitly provided instruction pointer
 * @param ip Failing instruction pointer
 * @param code Error code
 * @param arg1 Argument 1
 * @param arg2 Argument 2
 * @param arg3 Argument 3
 * @param arg4 Argument 4
 * @attention This function never returns
*/
EXTERN NORETURN void KePanicIPEx(uintptr_t ip, uintptr_t code, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3, uintptr_t arg4);


/**
 * @}
*/

#endif