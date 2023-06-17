#ifndef KERNEL32_IT_H_
#define KERNEL32_IT_H_

/**
 * @file it.h
 * @brief Kernel interrupt module
 * 
 * Handles exceptions and interrupts.
 * Support hardware interrupts with APIC and IOAPIC.
*/

#include <stdint.h>
#include <stdbool.h>
#include "../defines.h"

/**
 * @brief Attribute to be used with interrupt handlers
*/
#define IT_HANDLER_ATTRIBUTES __attribute__ ((interrupt, target("general-regs-only")))

/**
 * @brief Enum containing exception vectors
*/
enum It_exceptionVector
{
    IT_EXCEPTION_DIVIDE = 0,
    IT_EXCEPTION_DEBUG = 1,
    IT_EXCEPTION_NMI = 2,
    IT_EXCEPTION_BREAKPOINT = 3,
    IT_EXCEPTION_OVERFLOW = 4,
    IT_EXCEPTION_BOUND_EXCEEDED = 5,
    IT_EXCEPTION_IVALID_OPCODE = 6,
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
kError_t It_init(void);

/**
 * @brief Set up interrupt (not exception) handler
 * @param vector Interrupt vector number (>=32)
 * @param isr Interrupt service routine pointer
 * @param kernelModeOnly True for this interrupt to be available ine kernel mode exclusively
 * @return Error code
*/
kError_t It_setInterruptHandler(uint8_t vector, void *isr, bool kernelModeOnly);

/**
 * @brief Set up exception (not interrupt) handler
 * @param vector Exception vector number (<32)
 * @param isr Interrupt service routine pointer
 * @return Error code
*/
kError_t It_setExceptionHandler(enum It_exceptionVector vector, void *isr);

/**
 * @brief Disable (outdated) PIC - APIC and IOAPIC should be used instead
 * @attention This is done on interrupt module initialization
 * @return Error code
*/
kError_t It_disablePIC(void);

#endif
