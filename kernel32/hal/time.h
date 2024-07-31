#ifndef HAL_TIME_H_
#define HAL_TIME_H_

/**
 * @file time.h
 * @brief Time HAL module
 * 
 * @defgroup time Times HAL driver
 * @ingroup hal
*/

#include "defines.h"
#include <stdint.h>

/**
 * @addtogroup time
 * @{
*/

/**
 * @brief Initialize date and time module
 * @return Status code
*/
INTERNAL STATUS HalInitTimeController(void);

EXPORT
/**
 * @brief Get timestamp in nanoseconds
 * @return Timestamp in ns
*/
EXTERN uint64_t HalGetTimestamp(void);

EXPORT
/**
 * @brief Get timestamp in microseconds
 * @return Timestamp in us
*/
EXTERN uint64_t HalGetTimestampMicros(void);

EXPORT
/**
 * @brief Get timestamp in milliseconds
 * @return Timestamp in ms
*/
EXTERN uint64_t HalGetTimestampMillis(void);

/**
 * @brief Initialize system (shcheduler) timer
 * @param vector Interrupt vector number
 * @return Status code
*/
INTERNAL STATUS HalConfigureSystemTimer(uint8_t vector);

/**
 * @brief Start one-shot system timer
 * @param time Time in microseconds
 * @return Status code
*/
INTERNAL STATUS HalStartSystemTimer(uint64_t time);

/**
 * @}
*/

#endif