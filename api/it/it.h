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
#define IT_VECTOR_ANY 0

/**
 * @brief Lowest vector available for IRQs
*/
#define IT_IRQ_VECTOR_BASE 48

/**
 * @brief First vector available for interrupt requests
*/
#define IT_FIRST_INTERRUPT_VECTOR 32

#define IT_MAX_IRQ_VECTORS (256 - IT_FIRST_INTERRUPT_VECTOR)

/**
 * @brief Maximum number of shared IRQ consumers
*/
#define IT_MAX_SHARED_IRQ_CONSUMERS 8

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
extern uint8_t ItReserveVector(uint8_t vector);

/**
 * @brief Free reserved vector
 * @param vector Reserved vector number to be freed
 * @note This function affects only vector that were reserved but not used
*/
extern void ItFreeVector(uint8_t vector);

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
extern STATUS ItInstallInterruptHandler(uint8_t vector, ItHandler isr, void *context);

/**
 * @brief Uninstall interrupt handler
 * @param vector Vector number
 * @param isr Interrupt service routine pointer
 * @return Status code
*/
extern STATUS ItUninstallInterruptHandler(uint8_t vector, ItHandler isr);

/**
 * @brief Enable/disable interrupt handler
 * @param vector Vector number
 * @param isr Interrupt service routine pointer
 * @param enable Enable state: \a true to enable, \a false to disable
 * @return Status code
 * @warning This function does not install the interrupt handler
*/
extern STATUS ItSetInterruptHandlerEnable(uint8_t vector, ItHandler isr, bool enable);


#ifdef __cplusplus
}
#endif

#endif