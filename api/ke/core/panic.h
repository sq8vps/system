//This header file is generated automatically
#ifndef EXPORTED___API__KE_CORE_PANIC_H_
#define EXPORTED___API__KE_CORE_PANIC_H_

#ifdef __cplusplus
extern "C" 
{
#endif

#include <stdint.h>
#include "defines.h"
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
    PRIORITY_LEVEL_TOO_LOW,
    PRIORITY_LEVEL_TOO_HIGH,
    RP_FINALIZED_OUT_OF_LINE,
    ILLEGAL_PRIORITY_LEVEL_CHANGE,
    ILLEGAL_PRIORITY_LEVEL,
};

/**
 * @brief Emergency system shutdown routine - kernel panic
 * @param code Error code
 * @attention This function never returns
*/
extern NORETURN void KePanic(uintptr_t code);

/**
 * @brief Emergency system shutdown routine - kernel panic
 * @param code Error code
 * @param arg1 Argument 1
 * @param arg2 Argument 2
 * @param arg3 Argument 3
 * @param arg4 Argument 4
 * @attention This function never returns
*/
extern NORETURN void KePanicEx(uintptr_t code, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3, uintptr_t arg4);

/**
 * @brief Emergency system shutdown routine - kernel panic with explicitly provided instruction pointer
 * @param ip Failing instruction pointer
 * @param code Error code
 * @attention This function never returns
*/
extern NORETURN void KePanicIP(uintptr_t ip, uintptr_t code);

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
extern NORETURN void KePanicIPEx(uintptr_t ip, uintptr_t code, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3, uintptr_t arg4);


#ifdef __cplusplus
}
#endif

#endif