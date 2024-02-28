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
#include "it/it.h"
#define HAL_INTERRUPT_INPUT_ANY UINT32_MAX

enum HalInterruptMethod
{
    IT_METHOD_NONE,
    IT_METHOD_PIC,
    IT_METHOD_APIC,
};

/**
 * @brief IRQ polarity
*/
enum HalInterruptPolarity
{
    IT_POLARITY_ACTIVE_LOW,
    IT_POLARITY_ACTIVE_HIGH,
};

/**
 * @brief IRQ trigger mode
*/
enum HalInterruptTrigger
{
    IT_TRIGGER_EDGE,
    IT_TRIGGER_LEVEL,
};

/**
 * @brief IRQ wake capability
*/
enum HalInterruptWakeCapable
{
    IT_WAKE_INCAPABLE,
    IT_WAKE_CAPABLE,
};

/**
 * @brief IRQ sharing capability
*/
enum HalInterruptSharing
{
    IT_NOT_SHAREABLE,
    IT_SHAREABLE,
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

struct HalInterruptParams
{
    enum HalInterruptMode mode;
    enum HalInterruptPolarity polarity;
    enum HalInterruptTrigger trigger;
    enum HalInterruptSharing shared;
    enum HalInterruptWakeCapable wake;
};

/**
 * @brief Resolve legacy ISA IRQ to global interrupt mapping
 * @param irq ISA IRQ from device
 * @return Resolved IRQ after remapping (if applicable)
*/
extern uint32_t HalResolveIsaIrqMapping(uint32_t irq);

/**
 * @brief Register external IRQ
 * @param input Global interrupt source number
 * @param isr Interrupt service routine pointer
 * @param *context Context to be passed to ISR
 * @param params IRQ parameters
 * @return Status code
*/
extern STATUS HalRegisterIrq(
    uint32_t input,
    ItHandler isr,
    void *context,
    struct HalInterruptParams params);

/**
 * @brief Unregister external IRQ
 * @param input Global interrupt source number
 * @param isr Interrupt service routine pointer
 * @return Status code
*/
extern STATUS HalUnregisterIrq(uint32_t input, ItHandler isr);

/**
 * @brief Reserve interrupt input
 * @param input Requested input or \a HAL_INTERRUPT_INPUT_ANY
 * @return Reserved interrupt input or \a UINT32_MAX on failure
*/
extern uint32_t HalReserveIrq(uint32_t input);

/**
 * @brief Free previously interrupt input
 * @param input Previously reserved interrupt input
*/
extern void HalFreeIrq(uint32_t input);

/**
 * @brief Get vector number associated with given interrupt source
 * @param input Global interrupt source number
 * @return Associated vector number or 0 on failure
*/
extern uint8_t HalGetAssociatedVector(uint32_t input);

/**
 * @brief Enable external interrupt
 * @param input IRQ (input) number
 * @param isr Interrupt service routine pointer
 * @return Error code
*/
extern STATUS HalEnableIrq(uint32_t input, ItHandler isr);

/**
 * @brief Disable external interrupt
 * @param input IRQ (input) number
 * @param isr Interrupt service routine pointer
 * @return Error code
*/
extern STATUS HalDisableIrq(uint32_t input, ItHandler isr);

/**
 * @brief Clear external interrupt flag
 * @param input IRQ (input) number
 * @return Error code
*/
extern STATUS HalClearInterruptFlag(uint32_t input);

/**
 * @brief Get interrupt handling method
 * @return Interrupt handling method
*/
extern enum HalInterruptMethod HalGetInterruptHandlingMethod(void);


#ifdef __cplusplus
}
#endif

#endif