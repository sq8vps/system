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
*/

enum HalInterruptMethod
{
    IT_METHOD_NONE,
    IT_METHOD_PIC,
    IT_METHOD_APIC,
};

enum HalInterruptPolarity
{
    IT_POLARITY_ACTIVE_LOW,
    IT_POLARITY_ACTIVE_HIGH,
};

enum HalInterruptTrigger
{
    IT_TRIGGER_EDGE,
    IT_TRIGGER_LEVEL,
};

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
STATUS HalInitInterruptController(void);

/**
 * @brief Enable external interrupt
 * @param irq Interrupt vector
 * @return Error code
*/
EXPORT STATUS HalEnableIRQ(uint8_t irq);

/**
 * @brief Disable external interrupt
 * @param irq Interrupt vector
 * @return Error code
*/
EXPORT STATUS HalDisableIRQ(uint8_t irq);

/**
 * @brief Clear external interrupt flag
 * @param irq Interrupt vector
 * @return Error code
*/
EXPORT STATUS HalClearInterruptFlag(uint8_t irq);

/**
 * @brief Initialize system (shcheduler) timer
 * @param interval Interval in milliseconds
 * @param irq Interrupt vector number
 * @return Status code
*/
STATUS HalInitSystemTimer(uint32_t interval, uint8_t irq);

/**
 * @brief Restart system timer (start couting again)
 * @return Status code
*/
STATUS HalRestartSystemTimer();

/**
 * @brief Get interrupt handling method
 * @return Interrupt handling method
*/
enum HalInterruptMethod HalGetInterruptHandlingMethod(void);

/**
 * @}
*/

#endif