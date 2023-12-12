//This header file is generated automatically
#ifndef EXPORTED___API__HAL_INTERRUPT_H_
#define EXPORTED___API__HAL_INTERRUPT_H_

#ifdef __cplusplus
extern "C" 
{
#endif

#include <stdint.h>
#include "defines.h"
#include <stdbool.h>
enum HalInterruptMethod
{
    IT_METHOD_NONE,
    IT_METHOD_PIC,
    IT_METHOD_APIC,
};

/**
 * @brief External interrupt polarity
*/
enum HalInterruptPolarity
{
    IT_POLARITY_ACTIVE_LOW,
    IT_POLARITY_ACTIVE_HIGH,
};

/**
 * @brief External interrupt trigger mode
*/
enum HalInterruptTrigger
{
    IT_TRIGGER_EDGE,
    IT_TRIGGER_LEVEL,
};

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
 * @brief Register external IRQ
 * @param input Global interrupt source number
 * @param vector Interrupt vector number
 * @param mode Interrupt delivery mode
 * @param polarity Input polarity
 * @param trigger Interrupt trigger
 * @return Status code
*/
extern STATUS HalRegisterIRQ(uint32_t input, uint8_t vector, enum HalInterruptMode mode,
                        enum HalInterruptPolarity polarity, enum HalInterruptTrigger trigger);

/**
 * @brief Unregister external IRQ
 * @param input Global interrupt source number
 * @return Status code
*/
extern STATUS HalUnregisterIRQ(uint32_t input);

/**
 * @brief Get vector number associated with given interrupt source
 * @param input Global interrupt source number
 * @return Associated vector number or 0 on failure
*/
extern uint8_t HalGetAssociatedVector(uint32_t input);

/**
 * @brief Enable external interrupt
 * @param irq Interrupt vector
 * @return Error code
*/
extern STATUS HalEnableIRQ(uint8_t irq);

/**
 * @brief Disable external interrupt
 * @param irq Interrupt vector
 * @return Error code
*/
extern STATUS HalDisableIRQ(uint8_t irq);

/**
 * @brief Clear external interrupt flag
 * @param irq Interrupt vector
 * @return Error code
*/
extern STATUS HalClearInterruptFlag(uint8_t irq);


#ifdef __cplusplus
}
#endif

#endif