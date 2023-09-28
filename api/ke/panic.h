//This header file is generated automatically
#ifndef API___API__KE_PANIC_H_
#define API___API__KE_PANIC_H_

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
 * @brief Emergency system shutdown routine - kernel panic from interrupt
 * @param *moduleName Module name string (can be NULL)
 * @param ip Failing instruction pointer
 * @param code Error code
 * @attention This function never returns
*/
extern NORETURN void KePanicFromInterrupt(char *moduleName, uintptr_t ip, uintptr_t code);

/**
 * @brief Emergency system shutdown routine - kernel panic from interrupt
 * @param *moduleName Module name string (can be NULL)
 * @param ip Failing instruction pointer
 * @param code Error code
 * @param arg1 Argument 1
 * @param arg2 Argument 2
 * @param arg3 Argument 3
 * @param arg4 Argument 4
 * @attention This function never returns
*/
extern NORETURN void KePanicFromInterruptEx(char *moduleName, uintptr_t ip, uintptr_t code, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3, uintptr_t arg4);


#ifdef __cplusplus
}
#endif

#endif