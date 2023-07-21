#ifndef KERNEL_IT_H_
#define KERNEL_IT_H_

/**
 * @file it.h
 * @brief Kernel interrupt module
 * 
 * Handles exceptions and interrupts.
 * 
 * @defgroup it Interrupt module
*/

#include <stdint.h>
#include <stdbool.h>
#include "defines.h"

#define IT_FIRST_INTERRUPT_VECTOR 32

enum ItLegacyVectors
{
    IT_LEGACY_PIT_VECTOR = 32,
};

/**
 * @brief Attribute to be used with interrupt handlers
*/
#define IT_HANDLER __attribute__ ((interrupt, target("general-regs-only")))

/**
 * @brief Enum containing exception vectors
*/
enum ItExceptionVector
{
    IT_EXCEPTION_DIVIDE = 0,
    IT_EXCEPTION_DEBUG = 1,
    IT_EXCEPTION_NMI = 2,
    IT_EXCEPTION_BREAKPOINT = 3,
    IT_EXCEPTION_OVERFLOW = 4,
    IT_EXCEPTION_BOUND_EXCEEDED = 5,
    IT_EXCEPTION_INVALID_OPCODE = 6,
    IT_EXCEPTION_DEVICE_UNAVAILABLE = 7,
    IT_EXCEPTION_DOUBLE_FAULT = 8,
    IT_EXCEPTION_COPROCESSOR_OVERRUN = 9,
    IT_EXCEPTION_INVALID_TSS = 10,
    IT_EXCEPTION_SEGMENT_NOT_PRESENT = 11,
    IT_EXCEPTION_STACK_FAULT = 12,
    IT_EXCEPTION_GENERAL_PROTECTION = 13,
    IT_EXCEPTION_PAGE_FAULT = 14,
    IT_EXCEPTION_FPU_ERROR = 16,
    IT_EXCEPTION_ALIGNMENT_CHECK = 17,
    IT_EXCEPTION_MACHINE_CHECK = 18,
    IT_EXCEPTION_SIMD_FPU = 19,
    IT_EXCEPTION_VIRTUALIZATION = 20,
    IT_EXCEPTION_CONTROL_PROTECTION = 21,
};

/**
 * @defgroup itConfig Interrupt module configuration routines and structures
 * @ingroup it
 * @{
*/


/**
 * @brief ISR frame for interrupts and exceptions without an error code with no privilege level change
*/
struct ItFrame
{
    uint32_t ip;
    uint32_t cs;
    uint32_t flags;
} __attribute__((packed));

/**
 * @brief ISR frame for exceptions with error code with no priviliege level change
*/
struct ItFrameEC
{
    uint32_t error;
	uint32_t ip;
    uint32_t cs;
    uint32_t flags;
} __attribute__((packed));

/**
 * @brief ISR frame for interrupts and exceptions without an error code with privilege level change
*/
struct ItFrameMS
{
    uint32_t ip;
    uint32_t cs;
    uint32_t flags;
    uint32_t esp;
    uint32_t ss;
} __attribute__((packed));

/**
 * @brief ISR frame for exceptions with error code with priviliege level change
*/
struct ItFrameECMS
{
    uint32_t error;
	uint32_t ip;
    uint32_t cs;
    uint32_t flags;
    uint32_t esp;
    uint32_t ss;
} __attribute__((packed));



/**
 * @brief Set up interrupts and assign default handlers to them
 * @return Error code
*/
STATUS ItInit(void);

/**
 * @brief Get free interrupt vector number
 * @return Free vector number or 0 on failure
*/
EXPORT uint8_t ItGetFreeVector(void);

/**
 * @brief Install interrupt handler
 * @param vector Interrupt vector number
 * @param isr Interrupt service routine pointer
 * @param cpl Required privilege level to use this interrupt
 * @return Status code
*/
EXPORT STATUS ItInstallInterruptHandler(uint8_t vector, void *isr, PrivilegeLevel_t cpl);

/**
 * @brief Uninstall interrupt handler
 * @param vector Vector number
 * @return Status code
*/
EXPORT STATUS ItUninstallInterruptHandler(uint8_t vector);

/**
 * @brief Check if exeception/interrupt was caused by kernel mode code
 * @param cs Code segment of failing code (from interrupt frame)
 * @return True if caused by kernel mode
*/
bool ItIsCausedByKernelMode(uint32_t cs);

/**
 * @brief Perform a hard CPU reset
 * @warning DO NOT USE. Use KePanic() and KePanicEx() instead.
*/
NORETURN void ItHardReset(void);

/**
 * @}
*/


#endif
