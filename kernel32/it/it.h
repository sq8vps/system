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

EXPORT
#define IT_VECTOR_ANY 0

EXPORT
/**
 * @brief Lowest vector available for IRQs
*/
#define IT_IRQ_VECTOR_BASE 48

EXPORT
/**
 * @brief First vector available for interrupt requests
*/
#define IT_FIRST_INTERRUPT_VECTOR 32

EXPORT
#define IT_MAX_IRQ_VECTORS (256 - IT_FIRST_INTERRUPT_VECTOR)

/*
* Vectors reserved and used by the kernel
*/
#define IT_SYSTEM_TIMER_VECTOR IT_FIRST_INTERRUPT_VECTOR
#define IT_LAPIC_SPURIOUS_VECTOR (IT_FIRST_INTERRUPT_VECTOR | 0xF)

EXPORT
/**
 * @brief Maximum number of shared IRQ consumers
*/
#define IT_MAX_SHARED_IRQ_CONSUMERS 8

/**
 * @brief Attribute to be used with interrupt handler wrappers
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
 * @brief ISR frame for interrupts and exceptions with no privilege level change
*/
struct ItFrame
{
    uint32_t ip;
    uint32_t cs;
    uint32_t flags;
} PACKED;

EXPORT
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

EXPORT
/**
 * @brief Type definition for generic interrupt service routine
*/
typedef STATUS (*ItHandler)(void *context);

/**
 * @brief Set up interrupts and assign default handlers to them
 * @return Error code
*/
INTERNAL STATUS ItInit(void);

EXPORT
/**
 * @brief Get and reserve vector
 * @param vector Requested vector number or \a IT_VECTOR_ANY for any vector number
 * @return Reserved vector or 0 on failure
 * @note Use \a ItFreeVector to release vector that was reserved but not used
*/
EXTERN uint8_t ItReserveVector(uint8_t vector);

EXPORT
/**
 * @brief Free reserved vector
 * @param vector Reserved vector number to be freed
 * @note This function affects only vector that were reserved but not used
*/
EXTERN void ItFreeVector(uint8_t vector);

EXPORT 
/**
 * @brief Install interrupt handler
 * @param vector Interrupt vector number
 * @param isr Interrupt service routine pointer
 * @param *context Context to be passed to ISR
 * @return Status code
 * @note This function fails if maximum number of shared IRQ consumers is already reached
 * @warning This function does not know anything about the shareability of the IRQs services by this vector
*/
EXTERN STATUS ItInstallInterruptHandler(uint8_t vector, ItHandler isr, void *context);

EXPORT 
/**
 * @brief Uninstall interrupt handler
 * @param vector Vector number
 * @param isr Interrupt service routine pointer
 * @return Status code
*/
STATUS ItUninstallInterruptHandler(uint8_t vector, ItHandler isr);

/**
 * @brief Check if exeception/interrupt was caused by kernel mode code
 * @param cs Code segment of failing code (from interrupt frame)
 * @return True if caused by kernel mode
*/
INTERNAL bool ItIsCausedByKernelMode(uint32_t cs);

/**
 * @brief Perform a hard CPU reset
 * @warning DO NOT USE. Use KePanic() and KePanicEx() instead.
*/
INTERNAL NORETURN void ItHardReset(void);

/**
 * @brief Disable all interrupts
*/
INTERNAL void ItDisableInterrupts(void);

/**
 * @brief Enable all interrupts
*/
INTERNAL void ItEnableInterrupts(void);


/**
 * @}
*/


#endif
