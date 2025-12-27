/**
 * @file time.h
 * @brief timekeeping abstraction layer
 * @ingroup hal
*/

#ifndef HAL_TIME_H_
#define HAL_TIME_H_

#include "defines.h"
#include <stdint.h>

/**
 * @addtogroup hal_time Timekeeping support
 * @brief Timekeeping support structures, definitions, and routines
 * @ingroup hal
 * 
 * This module provides an universal abstraction layer for time sources. It maintains a list of registered time sources and selects
 * the best (most accurate) one.
 * @{
*/

EXPORT_API

/**
 * @brief Clock source name length limit (excluding terminator)
 */
#define HAL_CLOCK_SOURCE_NAME_LENGTH 15

/**
 * @brief Clock source description structure
 */
struct HalClockSource
{
    char name[HAL_CLOCK_SOURCE_NAME_LENGTH + 1]; /**< Clock source name, up to 15 characters */
    uint64_t frequency; /**< Frequency in Hz */
    uint32_t rating; /**< Clock source rating - higher = better */
    uint64_t (*read)(void *context); /**< Get tick/cycles function */
    void *context; /**< Context for get tick/cycles function */
    void *control; /**< Internal associated clock source control structure */
};

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

/**
 * @brief Register new clock source
 * @param *cs Pre-filled clock source structure. This structure must be kept by the driver for all time.
 * @return Status code
 */
STATUS HalRegisterClockSource(struct HalClockSource *cs);

/**
 * @brief Update clock source state
 * @param *cs Clock source structure
 */
void HalUpdateClockSource(struct HalClockSource *cs);

END_EXPORT_API

/**
 * @brief Initialize system (scheduler) timer
 * @param vector Interrupt vector number
 * @kinternal
 * @return Status code
*/
INTERNAL STATUS HalConfigureSystemTimer(uint8_t vector);

/**
 * @brief Start one-shot system timer
 * @param time Time in microseconds
 * @kinternal
 * @return Status code
*/
INTERNAL STATUS HalStartSystemTimer(uint64_t time);

/**
 * @}
*/

#endif