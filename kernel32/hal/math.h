/**
 * @file math.h
 * @brief Math coprocessor support
 * @ingroup hal
*/

#ifndef HAL_MATH_H_
#define HAL_MATH_H_

#include "defines.h"

struct KeTaskControlBlock;

/**
 * @addtogroup hal_math Math coprocessor support module
 * @kinternal
 * @ingroup hal
 * @{
*/


/**
 * @brief Create buffer for coprocessor state storage
 * @return Buffer pointer or NULL on failure
*/
INTERNAL void *HalCreateMathStateBuffer(void);

/**
 * @brief Destroy coprocessor state storage buffer
 * @param *math Buffer pointer
*/
INTERNAL void HalDestroyMathStateBuffer(const void *math);

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