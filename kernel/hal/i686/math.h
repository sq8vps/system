/**
 * @file math.h
 * @brief Math coprocessor abstraction
 * @ingroup i686
 * @note This driver implements the universal HAL interface and most of its function are available using kernel API.
 */

#ifndef I686_MATH_H_
#define I686_MATH_H_

#include "defines.h"

/**
 * @addtogroup i686
 * @{
 */

/**
 * @brief Initialize FPU/SSE/AVX
 * @return Status code
 * @kinternal
 */
INTERNAL STATUS I686InitMath(void);

/**
 * @}
 */

#endif