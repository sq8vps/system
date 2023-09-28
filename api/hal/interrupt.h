//This header file is generated automatically
#ifndef EXPORTED___API__HAL_INTERRUPT_H_
#define EXPORTED___API__HAL_INTERRUPT_H_

#ifdef __cplusplus
extern "C" 
{
#endif

#include <stdint.h>
#include "defines.h"
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