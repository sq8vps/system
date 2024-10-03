#ifndef I686_IT_H_
#define I686_IT_H_

#include "defines.h"

/**
 * @brief Initialize Interrupt Descriptor Tables
 * @return Status code
 */
INTERNAL STATUS I686InitIdt(void);

/**
 * @brief Install IDT for current processor
 * @param cpu Current processor number
 */
INTERNAL void I686InstallIdt(uint16_t cpu);

/**
 * @brief Install exception handler
 * @param cpu CPU number
 * @param vector Exception vector
 * @param *isr Interrupt Service Routine
 * @return Status code
 */
INTERNAL STATUS I686InstallExceptionHandler(uint16_t cpu, uint8_t vector, void *isr);

#endif