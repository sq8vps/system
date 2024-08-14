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

EXPORT_API

/**
 * @brief Get timestamp in nanoseconds
 * @return Timestamp in ns
*/
uint64_t HalGetTimestamp(void);


/**
 * @brief Get timestamp in microseconds
 * @return Timestamp in us
*/
uint64_t HalGetTimestampMicros(void);


/**
 * @brief Get timestamp in milliseconds
 * @return Timestamp in ms
*/
uint64_t HalGetTimestampMillis(void);

END_EXPORT_API

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