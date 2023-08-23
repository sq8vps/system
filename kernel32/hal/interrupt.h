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

/**
 * @addtogroup halIt
 * @{
*/

enum HalInterruptMethod
{
    IT_METHOD_NONE,
    //IT_METHOD_PIC,
    IT_METHOD_APIC,
};

EXPORT
/**
 * @brief External interrupt polarity
*/
enum HalInterruptPolarity
{
    IT_POLARITY_ACTIVE_LOW,
    IT_POLARITY_ACTIVE_HIGH,
};

EXPORT
/**
 * @brief External interrupt trigger mode
*/
enum HalInterruptTrigger
{
    IT_TRIGGER_EDGE,
    IT_TRIGGER_LEVEL,
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

/**
 * @brief Initialize interrupt controller
*/
INTERNAL STATUS HalInitInterruptController(void);

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
 * @brief Get interrupt handling method
 * @return Interrupt handling method
*/
enum HalInterruptMethod HalGetInterruptHandlingMethod(void);

/**
 * @}
*/

#endif