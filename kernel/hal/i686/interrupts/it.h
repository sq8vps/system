/**
 * @file it.h
 * @brief IDT support module
 * @ingroup i686_it
 */

#ifndef I686_IT_H_
#define I686_IT_H_

#include "defines.h"

/**
 * @addtogroup i686_it IDT, exception, and low-level interrupt handling
 * @brief IDT, exception, and low-level interrupt handling
 * @kinternal
 * @ingroup i686
 * @{
 */

/**
 * @brief Initialize Interrupt Descriptor Tables
 * @return Status code
 */
INTERNAL STATUS I686InitIdt(void);

/**
 * @brief Install IDT for current processor
 * @param cpu Current processor number
 */
INTERNAL void I686InstallIdt(uint32_t cpu);

/**
 * @brief Install exception handler
 * @param cpu CPU number
 * @param vector Exception vector
 * @param *isr Interrupt Service Routine
 * @return Status code
 */
INTERNAL STATUS I686InstallExceptionHandler(uint32_t cpu, uint8_t vector, void *isr);

/**
 * @}
 */

#endif