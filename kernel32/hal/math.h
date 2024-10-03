#ifndef HAL_MATH_H_
#define HAL_MATH_H_

/**
 * @file math.h
 * @brief Math coprocessor HAL module
 * 
 * @defgroup math Coprocessor HAL driver
 * @ingroup hal
*/

#include "defines.h"

struct KeTaskControlBlock;

/**
 * @addtogroup math
 * @{
*/


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