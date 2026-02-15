/**
 * @file panic.h
 * @brief Kernel panic routines
 */

#ifndef KERNEL_PANIC_H_
#define KERNEL_PANIC_H_

#include <stdint.h>
#include "defines.h"
#include "io/log/syslog.h"

/**
 * @addtogroup ke_core
 * @{
*/

DRIVER_API

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
     * @note This is rather an unexpected error
     * 
     * - Arg 0: Index of the CPU that had no task to be executed
     * - Arg 1: Reserved
     * - Arg 2: Reserved
     * - Arg 3: Reserved
     */
    NO_EXECUTABLE_TASK = 0x3,
    /**
     * @brief A mutex-like object was released more times than it was acquired
     * 
     * - Arg 0: Object type:
     *      - 0: spinlock
     *      - 1: mutex
     *      - 2: semaphore
     *      - 3: R/W lock
     * - Arg 1: Failing object pointer
     * - Arg 2:
     *      - For semaphore (arg 0 = 2): current semaphore count (units)
     *      - For objects other than semaphore (arg 0 != 2): reserved
     * - Arg 3:
     *      - For semaphore (arg 0 = 2): number of units to be released
     *      - For objects other than semaphore (arg 0 != 2): reserved
     */
    UNACQUIRED_MUTEX_RELEASED = 0x4,
    /**
     * @brief More units were acquired for a mutex-like object than were available
     * 
     * - Arg 0: Object type:
     *      - 0: spinlock
     * - Arg 1: Failing object pointer
     * - Arg 2: Reserved
     * - Arg 3: Reserved
     * 
     * This error is probably caused by a programming error
     * where driver tried to acquire the same spinlock twice without releasing it.
     */
    BUSY_MUTEX_ACQUIRED = 0x5,
    /**
     * @brief An unknown or undefined fault occured
     * 
     * Drivers and HAL may provide additional context-dependent arguments.
     */
    UNEXPECTED_FAULT = 0x6,
    /**
     * @brief A fatal driver-defined error occured
     * 
     * Drivers can use this code to report catastrophic errors that have no predefined type.
     * The arguments are driver-dependent.
     */
    DRIVER_FATAL_ERROR = 0x7,
    /**
     * @brief Current priority level is too low to execute the operation
     * 
     * - Arg 0: Current priority level
     * - Arg 1: Minimum required priority level
     * - Arg 2: Reserved
     * - Arg 3: Reserved
     */
    PRIORITY_LEVEL_TOO_LOW = 0x8,
    /**
     * @brief Current priority level is too high to execute the operation
     * 
     * - Arg 0: Current priority level
     * - Arg 1: Maximum allowed priority level
     * - Arg 2: Reserved
     * - Arg 3: Reserved
     */
    PRIORITY_LEVEL_TOO_HIGH = 0x9,
    /**
     * @brief A previously queued Request Packet was finalized out of order
     * 
     * - Arg 0: Request Packet pointer
     * - Arg 1: Associated Request Packet queue pointer
     * - Arg 2: Reserved
     * - Arg 3: Reserved
     * 
     * IoFinalizeRp() was called on a Request Packet that was still queued.
     */
    RP_FINALIZED_OUT_OF_ORDER = 0xA,
    /**
     * @brief An illegal priority level change occured
     * 
     * - Arg 0: Priority level change direction:
     *      - 0: Priority level raise
     *      - 1: Priority level lower
     * - Arg 1: Requested priority level
     * - Arg 2: Current priority level
     * - Arg 3: Reserved
     */
    ILLEGAL_PRIORITY_LEVEL_CHANGE = 0xB,
    /**
     * @brief An illegal priority level was requested
     * 
     * - Arg 0: Requested priority level
     * - Arg 1: Reserved
     * - Arg 2: Reserved
     * - Arg 3: Reserved
     */
    ILLEGAL_PRIORITY_LEVEL = 0xC,
    /**
     * @brief There was an attempt to lock the object that is not lockable
     * 
     * - Arg 0: Object pointer
     * - Arg 1: Reserved
     * - Arg 2: Reserved
     * - Arg 3: Reserved
     * 
     * An Object Manager routine was called on an object that has no associated object lock, that is,
     * the object header is missing or corrupted. This might be caused by using Object Manager routines
     * on statically created objects that were not initialized properly.
     */
    OBJECT_LOCK_UNAVAILABLE = 0xD,
    /**
     * @brief There was an attempt to access illegal memory
     * 
     * - Arg 0: Failing operation type:
     *      - 1: Page unmapping
     * - Arg 1: Virtual address
     * - Arg 2:
     *      - 0x1: Page is not mapped or already unmapped, but the associated page table is present
     *      - 0x2: Associated page table is not present
     * - Arg 3: Reserved
     */
    MEMORY_ACCESS_VIOLATION = 0xE,
    /**
     * @brief A task attachment request was invalid
     * 
     * - Arg 0: Error type:
     *      - 0 - source and target tasks are the same
     *      - 1 - source task is already attached to other task
     *      - 2 - source task is not a kernel mode thread
     * - Arg 1: Source task control block (attachee)
     * - Arg 2: Target task control block
     * - Arg 3: 
     *      - For Arg 0 = 1: Task to which the source task is currently attached
     *      - For Arg 0 != 1: Reserved
     */
    INVALID_TASK_ATTACHMENT_ATTEMPT = 0xF,

    /**
     * @brief An attempt to access a memory unavailable in current context was made
     * 
     * - Arg 0: Failing virtual address
     * - Arg 1: Page fault reason:
     *      - 0x0: Reserved
     *      - 0x1: Page not present
     *      - 0x2: Write to read only page
     *      - 0x3: Execution from non-executable page
     *      - 0x8: Page protection violation
     *      - 0x9: Malformed page table entry
     *      - 0x1000: Other reason
     * - Arg 2: Access mode:
     *      - 0x0: Read access
     *      - 0x1: Write access
     *      - 0x2: Execute access
     * - Arg 3: Actual page flags for failing address (see @ref MmMemoryFlags)
     */
    PAGE_FAULT = 0x10,

    /**
     * @brief No workin init program was found
     */
    NO_WORKING_INIT = 0x11,
};


/**
 * @brief Emergency system shutdown routine - kernel panic
 * @param code Error code
 * @attention This function never returns
*/
[[noreturn]] void KePanic(uintptr_t code);


/**
 * @brief Emergency system shutdown routine - kernel panic
 * @param code Error code
 * @param arg1 Argument 1
 * @param arg2 Argument 2
 * @param arg3 Argument 3
 * @param arg4 Argument 4
 * @attention This function never returns
*/
[[noreturn]] void KePanicEx(uintptr_t code, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3, uintptr_t arg4);


/**
 * @brief Emergency system shutdown routine - kernel panic with explicitly provided instruction pointer
 * @param ip Failing instruction pointer
 * @param code Error code
 * @attention This function never returns
*/
[[noreturn]] void KePanicIP(uintptr_t ip, uintptr_t code);


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
[[noreturn]] void KePanicIPEx(uintptr_t ip, uintptr_t code, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3, uintptr_t arg4);

END_DRIVER_API

/**
 * @brief Print message and halt on boot failure
 * @param str Message to be printed
 * @kinternal
 */
#define FAIL_BOOT(str) do{LOG(SYSLOG_ERROR, "Boot failed: %s", str); while(1) {HALT();};} while(0);

/**
 * @}
*/

#endif