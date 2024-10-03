#ifndef I686_EXCEPTIONS_H_
#define I686_EXCEPTIONS_H_

#include <stdint.h>
#include "defines.h"

#if defined(__i686__)

/**
 * @brief ISR frame for interrupts and exceptions with no privilege level change
*/
struct ItFrame
{
    uint32_t ip;
    uint32_t cs;
    uint32_t flags;
} PACKED;


/**
 * @brief ISR frame for interrupts and exceptions with privilege level change
*/
struct ItFrameMS
{
    uint32_t ip;
    uint32_t cs;
    uint32_t flags;
    uint32_t esp;
    uint32_t ss;
} PACKED;

#endif

#if defined(__i686__) || defined(__amd64__)

enum I686PanicCode
{
    /**
     * @brief A division by zero occured
     * 
     * This error is caused by division by zero in kernel-mode software, such as drivers.
     * It is usually caused by a programming error.
     */
    DIVIDE_ERROR = 0,
    /**
     * @brief A non-maskable interrupt was asserted
     * 
     * The non-maskable interrupt is asserted by hardware.
     * It is usually caused by a hardware failure.
     */
    NON_MASKABLE_INTERRUPT = 2,
    /**
     * @brief An INTO instruction was executed while the OF flag was set
     * 
     * This exception is raised when an INTO (opcode 0xCE) instruction was
     * executed while the overflow flag (OF) was set.
     * This exception raised in kernel mode causes kernel panic.
     */
    OVERFLOW = 4,
    /**
     * @brief A bound check of array index (BOUND instruction) failed
     * 
     * This exception is raised when the array index is out of bounds
     * when using BOUND instruction. In kernel mode, it results in kernel panic.
     */
    BOUND_RANGE_EXCEEDED = 5,
    /**
     * @brief An invalid instruction was encountered
     * 
     * This exception is raised when execution of an invalid 
     * instruction was attempted.
     * It can be caused by broken executables, programming errors
     * or running a program compiled for different architecture.
     * In kernel mode, it results in kernel panic.
     */
    INVALID_OPCODE = 6,
    /**
     * @brief The x87 FPU is not available
     * 
     * This exception is raised when the x87 FPU is not available
     * at given time. This error should not happen in this implementation.
     */
    FPU_NOT_AVAILABLE = 7,
    /**
     * @brief An exception occured while handling a previous exception
     * 
     * This exception is raised when there was an exception
     * when handling previous exception. It always results in kernel panic.
     */
    DOUBLE_FAULT = 8,
    COPROCESSOR_SEGMENT_OVERRUN = 9,
    /**
     * @brief An invalid Task State Segment was encountered     
     * 
     * There was an error related to Task State Segment.
     * It always results in kernel panic.
     */
    INVALID_TSS = 10,
    /**
     * @brief A memory segment or gate descriptor in GDT is not present
     * 
     * This exception is caused by missing segment descriptor in the GDT.
     * Since there is a flat memory model used, this exception
     * should never occur.
     */
    SEGMENT_NOT_PRESENT = 11,
    /**
     * @brief A stack fault occured
     * 
     * This exception is raised when a stack segment is too small
     * or not present (should not happen in this implementation),
     * or a cannonical violation was detected (in long mode). 
     * In kernel mode, it results in kernel panic.
     */
    STACK_FAULT = 12,
    /**
     * @brief A protection violation was detected
     * 
     * This exception is caused by different conditions,
     * such as exceeded segment limit, executing priviledged 
     * instruction in non-priviledged mode, etc.
     * Refer to Intel manual for details.
     * In kernel mode, it always results in kernel panic.
     */
    GENERAL_PROTECTION_FAULT = 13,
    /**
     * @brief A memory access was not possible
     * 
     * This exception is caused by multiple conditions, such
     * as missing memory page, paged-out memory, writing to read-only
     * page, etc.
     * It is usually caused by a programming error,
     * such as accessing null or dangling pointers.
     */
    PAGE_FAULT = 14,
    /**
     * @brief An internal machine error or a bus error occured
     * 
     * This exception always results in kernel panic.
     */
    MACHINE_CHECK = 18,
    VIRTUALIZATION_EXCEPTION = 20,
    CONTROL_PROTECTION_EXCEPTION = 21,

    /**
     * @brief An inter-processor interrupt delivery timed out
     * 
     * This exception occurs when a CPU issued an IPI,
     * and IPI was not delivered to the target CPU within a given time.
     * This exception always results in kernel panic.
     * Panic parameters:
     * - Arg 0 - Panic code = \a IPI_DELIVERY_TIMEOUT
     * - Arg 1 - source CPU ID
     * - Arg 2 - target CPU ID
     * - Arg 3 - IPI type from \a I686IpiType
     */
    IPI_DELIVERY_TIMEOUT = 32,

    /**
     * @brief The received inter-processor interrupt has unknown type
     * 
     * 
     * This exception always results in kernel panic.
     * Panic parameters:
     * - Arg 0 - Panic code = \a IPI_UNKNOWN_TYPE
     * - Arg 1 - source CPU ID
     * - Arg 2 - target CPU ID
     * - Arg 3 - IPI type
     */
    IPI_UNKNOWN_TYPE = 33,
    UNEXPECTED_INTEL_TRAP = 0xFFFFFFFF,
};

/**
 * @brief Install all exception handlers for given CPU
 */
INTERNAL void I686InstallAllExceptionHandlers(uint16_t cpu);

#endif

#endif