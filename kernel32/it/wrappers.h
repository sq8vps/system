#ifndef IT_WRAPPERS_H_
#define IT_WRAPPERS_H_

#include <stdint.h>
#include <stdbool.h>
#include "defines.h"
#include "it.h"

/**
 * @brief Handle IRQ with given vector
 * @param vector Vector number
 */
void ItHandleIrq(uint8_t vector);

/**
 * @brief Number of interrupt handles for IRQs
 */
#define IT_HANDLER_COUNT (IT_LAST_INTERRUPT_VECTOR - IT_FIRST_INTERRUPT_VECTOR + 1)

/**
 * @brief Helper macro to create interrupt wrapper function name
 */
#define IT_ISR_WRAPPER_NAME(n) ItIsrWrapper##n

/**
 * @brief Interrupt wrapper function definition macro
 */
#define IT_ISR_WRAPPER(n) IT_HANDLER static void IT_ISR_WRAPPER_NAME(n)(void *frame) { ItHandleIrq(n); }



/**
 * @brief Table of interrupt wrappers that must be 
 * created and filled by the architecture-specific code.
 */
extern void (*ItWrappers[IT_HANDLER_COUNT])(void *frame);

#endif