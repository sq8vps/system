#ifndef KERNEL_DRIVER_HAL
#define KERNEL_DRIVER_HAL

/**
 * @file hal.h
 * @brief Hardware Abstraction Layer driver general routines
 * 
 * Provides general routines for system HAL.
 * 
 * @defgroup hal Hardware Abstraction Layer driver
*/

#include <stdint.h>
#include "defines.h"
#include <stdbool.h>
#include "ke/task/task.h"

/**
 * @addtogroup hal
 * @{
*/

/**
 * @brief Initialize hardware abstraction layer
 * 
 * Initializes whole Hardware Abstraction Layer. Reads system tables and sets up all core peripherals.
 * 
 * @return Status code
*/
INTERNAL STATUS HalInit(void);

/**
 * @brief Initialize math peripherals
 * @return Status code
*/
INTERNAL STATUS HalInitMath(void);

/**
 * @brief Create buffer for coprocessor state storage
 * @return Buffer pointer or NULL on failure
*/
INTERNAL void *HalCreateMathStateBuffer(void);

/**
 * @brief Store coprocessor state
 * @param *tcb Task control block
*/
INTERNAL void HalStoreMathState(struct KeTaskControlBlock *tcb);

/**
 * @brief Restore coprocessor state
 * @param *tcb Task control block
*/
INTERNAL void HalRestoreMathState(struct KeTaskControlBlock *tcb);

/**
 * @}
*/

#endif