#ifndef KERNEL_INTERRUPT_H_
#define KERNEL_INTERRUPT_H_

/**
 * @file interrupt.h
 * @brief HAL Interrupt module
 * 
 * A HAL module providing external and internal interrupt handling routines and structures.
 * This module supports APIC and PIC as interrupt controllers.
 * PIT is used as a timer only when APIC is not present.
 * 
 * @ingroup hal
 * @defgroup halIt HAL Interrupt module
*/

#include <stdint.h>
#include "defines.h"
#include <stdbool.h>

/**
 * @addtogroup halIt
 * @{
*/

EXPORT
enum HalInterruptMethod
{
    IT_METHOD_NONE,
    IT_METHOD_PIC,
    IT_METHOD_APIC,
};

EXPORT
/**
 * @brief IRQ polarity
*/
enum HalInterruptPolarity
{
    IT_POLARITY_ACTIVE_LOW,
    IT_POLARITY_ACTIVE_HIGH,
};

EXPORT
/**
 * @brief IRQ trigger mode
*/
enum HalInterruptTrigger
{
    IT_TRIGGER_EDGE,
    IT_TRIGGER_LEVEL,
};

EXPORT
/**
 * @brief IRQ wake capability
*/
enum HalInterruptWakeCapable
{
    IT_WAKE_INCAPABLE,
    IT_WAKE_CAPABLE,
};

EXPORT
/**
 * @brief IRQ sharing capability
*/
enum HalInterruptSharing
{
    IT_NOT_SHARED,
    IT_SHARED,
};

EXPORT
/**
 * @brief External interrupt mode
*/
enum HalInterruptMode
{
    IT_MODE_FIXED,
    IT_MODE_LOWEST_PRIORITY,
    IT_MODE_SMI,
    IT_MODE_NMI,
    IT_MODE_INIT,
    IT_MODE_EXTINT,
};

EXPORT
struct HalInterruptParams
{
    enum HalInterruptMode mode;
    enum HalInterruptPolarity polarity;
    enum HalInterruptTrigger trigger;
    enum HalInterruptSharing shared;
    enum HalInterruptWakeCapable wake;
};


#define HAL_PIC_REMAP_VECTOR IT_IRQ_VECTOR_BASE

EXPORT
/**
 * @brief Resolve legacy ISA IRQ to global interrupt mapping
 * @param irq ISA IRQ from device
 * @return Resolved IRQ after remapping (if applicable)
*/
EXTERN uint32_t HalResolveIsaIrqMapping(uint32_t irq);

/**
 * @brief Set dual-PIC presence state
 * @param state Dual-PIC presence state
*/
INTERNAL void HalSetDualPicPresence(bool state);

/**
 * @brief Add ISA remap entry when I/O APIC is used
 * @param isaIrq Original ISA IRQ
 * @param gsi Global System Interrupt (Global IRQ number)
 * @return Status code
*/
INTERNAL STATUS HalAddIsaRemapEntry(uint8_t isaIrq, uint32_t gsi);

/**
 * @brief Initialize interrupt controller
*/
INTERNAL STATUS HalInitInterruptController(void);

EXPORT
/**
 * @brief Register external IRQ
 * @param input Global interrupt source number
 * @param vector Interrupt vector number
 * @param mode Interrupt delivery mode
 * @param polarity Input polarity
 * @param trigger Interrupt trigger
 * @return Status code
*/
EXTERN STATUS HalRegisterIRQ(uint32_t input, uint8_t vector, enum HalInterruptMode mode,
                        enum HalInterruptPolarity polarity, enum HalInterruptTrigger trigger);

EXPORT
/**
 * @brief Unregister external IRQ
 * @param input Global interrupt source number
 * @return Status code
*/
EXTERN STATUS HalUnregisterIRQ(uint32_t input);

EXPORT
/**
 * @brief Get vector number associated with given interrupt source
 * @param input Global interrupt source number
 * @return Associated vector number or 0 on failure
*/
EXTERN uint8_t HalGetAssociatedVector(uint32_t input);

EXPORT
/**
 * @brief Enable external interrupt
 * @param irq Interrupt vector
 * @return Error code
*/
EXTERN STATUS HalEnableIRQ(uint8_t irq);

EXPORT
/**
 * @brief Disable external interrupt
 * @param irq Interrupt vector
 * @return Error code
*/
EXTERN STATUS HalDisableIRQ(uint8_t irq);

EXPORT
/**
 * @brief Clear external interrupt flag
 * @param irq Interrupt vector
 * @return Error code
*/
EXTERN STATUS HalClearInterruptFlag(uint8_t irq);

/**
 * @brief Initialize system (shcheduler) timer
 * @param irq Interrupt vector number
 * @return Status code
*/
INTERNAL STATUS HalConfigureSystemTimer(uint8_t irq);

/**
 * @brief Start one-shot system timer
 * @param time Time in microseconds
 * @return Status code
*/
INTERNAL STATUS HalStartSystemTimer(uint64_t time);

/**
 * @brief Check if generated interrupt is spurious and should not be processed
 * @return True if spurious, false if not
*/
INTERNAL bool HalIsInterruptSpurious(void);

EXPORT
/**
 * @brief Get interrupt handling method
 * @return Interrupt handling method
*/
EXTERN enum HalInterruptMethod HalGetInterruptHandlingMethod(void);

#define HAL_TASK_PRIORITY_PASSIVE 0
#define HAL_TASK_PRIORITY_DPC 2

/**
 * @brief Set current task priority
 * @param priority New task priority (0-15)
 * @return Status code
*/
INTERNAL STATUS HalSetTaskPriority(uint8_t priority);

/**
 * @brief Get current task priority
 * @return Current task priority (0-15)
*/
INTERNAL uint8_t HalGetTaskPriority(void);

/**
 * @brief Get current processor priority
 * @return Current processor priority (0-15)
*/
INTERNAL uint8_t HalGetProcessorPriority(void);

/**
 * @}
*/

#endif