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
#include "io/disp/print.h"

/**
 * @defgroup panic Kernel panic routines
 * @ingroup ke
 * @{
*/

/**
 * @brief Get nth caller address
 * @param n Caller level: 0 - the caller of the given function
 * @warning This macro is GCC-specific and is unsafe for n>0
*/
#define KE_GET_CALLER_ADDRESS(n) (uintptr_t)__builtin_extract_return_addr(__builtin_return_address(n))

EXPORT
/**
 * @brief Main kernel panic error codes
*/
enum KernelPanicCode
{
    NO_ERROR = 0x0,
    /**
     * @brief An uncorrectable kernel mode fault occurred
     * 
     * An uncorrectable kernel mode fault occured. Refer to architecture-specific
     * error codes for details.
     */
    KERNEL_MODE_FAULT = 0x1,
    /**
     * @brief A critical error occured during early boot stage
     */
    BOOT_FAILURE = 0x2,
    /**
     * @brief No task to be executed is available
     * 
     * This error should not normally occur.
     */
    NO_EXECUTABLE_TASK = 0x3,
    /**
     * @brief A mutex-like object was released without being acquired first
     */
    UNACQUIRED_MUTEX_RELEASED = 0x4,
    /**
     * @brief A mutex-like object was acquired before being released
     */
    BUSY_MUTEX_ACQUIRED = 0x5,
    /**
     * @brief A unknown fault occured
     */
    UNEXPECTED_FAULT = 0x6,
    /**
     * @brief A fatal driver-defined error occured
     */
    DRIVER_FATAL_ERROR = 0x7,
    /**
     * @brief Current priority level is too low to execute the operation
     */
    PRIORITY_LEVEL_TOO_LOW = 0x8,
    /**
     * @brief Current priority level is too high to execute the operation
     */
    PRIORITY_LEVEL_TOO_HIGH = 0x9,
    /**
     * @brief A queued Request Packet was finalized out of line in a simple queue
     */
    RP_FINALIZED_OUT_OF_LINE = 0xA,
    /**
     * @brief An illegal priority level change occured
     */
    ILLEGAL_PRIORITY_LEVEL_CHANGE = 0xB,
    /**
     * @brief An illegal priority level was requested
     */
    ILLEGAL_PRIORITY_LEVEL = 0xC,
    /**
     * @brief There was an attempt to lock the object that is not lockable
     */
    OBJECT_LOCK_UNAVAILABLE = 0xD,
    /**
     * @brief There was an attempt to access illegal memory
     */
    MEMORY_ACCESS_VIOLATION = 0xE,
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
 * @brief Print message and halt on boot failure
 * @param str Message to be printed
 */
#define FAIL_BOOT(str) do{IoPrint("Boot failed: %s", str); while(1);} while(0);

/**
 * @}
*/

#endif