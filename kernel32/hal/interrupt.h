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
#include "it/it.h"

/**
 * @addtogroup halIt
 * @{
*/

EXPORT_API

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
 * @brief IRQ mode
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
uint32_t I686ResolveIsaIrqMapping(uint32_t irq);




/**
 * @brief Register external IRQ
 * @param input Global interrupt source number
 * @param isr Interrupt service routine pointer
 * @param *context Context to be passed to ISR
 * @param params IRQ parameters
 * @return Status code
*/
STATUS HalRegisterIrq(
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
STATUS HalUnregisterIrq(uint32_t input, ItHandler isr);


/**
 * @brief Reserve interrupt input
 * @param input Requested input or \a HAL_INTERRUPT_INPUT_ANY
 * @return Reserved interrupt input or \a UINT32_MAX on failure
*/
uint32_t HalReserveIrq(uint32_t input);


/**
 * @brief Free previously interrupt input
 * @param input Previously reserved interrupt input
*/
void HalFreeIrq(uint32_t input);


/**
 * @brief Get vector number associated with given interrupt source
 * @param input Global interrupt source number
 * @return Associated vector number or 0 on failure
*/
uint8_t HalGetAssociatedVector(uint32_t input);


/**
 * @brief Enable external interrupt
 * @param input IRQ (input) number
 * @param isr Interrupt service routine pointer
 * @return Error code
*/
STATUS HalEnableIrq(uint32_t input, ItHandler isr);


/**
 * @brief Disable external interrupt
 * @param input IRQ (input) number
 * @param isr Interrupt service routine pointer
 * @return Error code
*/
STATUS HalDisableIrq(uint32_t input, ItHandler isr);


/**
 * @brief Clear external interrupt flag
 * @param input IRQ (input) number
 * @return Error code
*/
STATUS HalClearInterruptFlag(uint32_t input);




/**
 * @brief Raise current task priority level
 * @param prio New priority level
 * @return Previous task priority level
 * @warning If new priority level is lower than current priority level, a kernel panic occurs
*/
PRIO HalRaisePriorityLevel(PRIO prio);


/**
 * @brief Lower current task priority level
 * @param prio Priority level returned from \a HalRaisePriorityLevel()
 * @warning If the provided priority level is higher than current priority level, a kernel panic occurs
*/
void HalLowerPriorityLevel(PRIO prio);


/**
 * @brief Get current task priority
 * @return Current task priority
*/
PRIO HalGetTaskPriority(void);


/**
 * @brief Get current processor priority
 * @return Current processor priority
*/
PRIO HalGetProcessorPriority(void);


/**
 * @brief Check if current priority level is within the specified range
 * 
 * This function checks if current priority level is within the given range,
 * that is, if \a lower <= \a current <= \a upper, and causes kernel panic if it is not.
 * Otherwise nothing happens.
 * @param lower Lower inclusive bound
 * @param upper Upper inclusive bound
 * @note On panic, this function passes the address of the caller of the caller of this function.
*/
void HalCheckPriorityLevel(PRIO lower, PRIO upper);

END_EXPORT_API

/**
 * @brief Set current task priority
 * @param prio Priority to be set
 */
INTERNAL void HalSetTaskPriority(PRIO prio);

/**
 * @brief Check if generated interrupt is spurious and should not be processed
 * @return True if spurious, false if not
*/
INTERNAL bool HalIsInterruptSpurious(void);

/**
 * @brief Add ISA remap entry when I/O APIC is used
 * @param isaIrq Original ISA IRQ
 * @param gsi Global System Interrupt (Global IRQ number)
 * @return Status code
*/
INTERNAL STATUS I686AddIsaRemapEntry(uint8_t isaIrq, uint32_t gsi);

/**
 * @}
*/

#endif