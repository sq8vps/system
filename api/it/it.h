//This header file is generated automatically
#ifndef EXPORTED___API__IT_IT_H_
#define EXPORTED___API__IT_IT_H_

#ifdef __cplusplus
extern "C" 
{
#endif

#include <stdint.h>
#include <stdbool.h>
#include "defines.h"

/**
 * @brief Magic value to be used when requesting random vector number
 */
#define IT_VECTOR_ANY 0



/**
 * @brief Maximum number of shared IRQ consumers
*/
#define IT_MAX_SHARED_IRQ_CONSUMERS 8

/**
 * @brief Attribute to be used with interrupt handler wrappers
*/
#define IT_HANDLER __attribute__ ((interrupt, target("general-regs-only")))


/**
 * @brief Type definition for generic interrupt service routine
*/
typedef STATUS (*ItHandler)(void *context);

/**
 * @brief Get and reserve vector
 * @param vector Requested vector number or \a IT_VECTOR_ANY for any vector number
 * @return Reserved vector or 0 on failure
 * @note Use \a ItFreeVector to release vector that was reserved but not used
*/
uint8_t ItReserveVector(uint8_t vector);


/**
 * @brief Free reserved vector
 * @param vector Reserved vector number to be freed
 * @note This function affects only vector that were reserved but not used
*/
void ItFreeVector(uint8_t vector);

 
/**
 * @brief Install interrupt handler
 * @param vector Interrupt vector number
 * @param isr Interrupt service routine pointer
 * @param *context Context to be passed to ISR
 * @return Status code
 * @note This function fails if maximum number of shared IRQ consumers is already reached
 * @warning This function does not know anything about the shareability of the IRQs services by this vector
 * @warning This function does not enable the interrupt handler
*/
STATUS ItInstallInterruptHandler(uint8_t vector, ItHandler isr, void *context);

 
/**
 * @brief Uninstall interrupt handler
 * @param vector Vector number
 * @param isr Interrupt service routine pointer
 * @return Status code
*/
STATUS ItUninstallInterruptHandler(uint8_t vector, ItHandler isr);


/**
 * @brief Enable/disable interrupt handler
 * @param vector Vector number
 * @param isr Interrupt service routine pointer
 * @param enable Enable state: \a true to enable, \a false to disable
 * @return Status code
 * @warning This function does not install the interrupt handler
*/
STATUS ItSetInterruptHandlerEnable(uint8_t vector, ItHandler isr, bool enable);


#ifdef __cplusplus
}
#endif

#endif