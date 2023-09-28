#ifndef KERNEL_TIME_H_
#define KERNEL_TIME_H_

#include <stdint.h>
#include "defines.h"

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

#endif