/**
 * @file pit.h
 * @brief 8254 PIT driver
 * @ingroup i686
 */

#ifndef KERNEL_PIT_H_
#define KERNEL_PIT_H_

#include <stdint.h>
#include "defines.h"

/**
 * @addtogroup i686_pit 8254 PIT driver
 * @brief 8254 PIT driver
 * @ingroup i686
 * @kinternal
 * 
 * PIT is not used in Nabla kernel as a time source. Here it is used in single-shot mode for calibration of 
 * other timers.
 * @{
 */

/**
 * @brief Initialize Programmable Interval Timer
*/
INTERNAL void PitInit(void);

/**
 * @brief Set PIT (channel 0) interval
 * @param interval Interval in milliseconds
*/
INTERNAL void PitSetInterval(uint32_t interval);

/**
 * @brief Callback used for single-shot PIT measurement, called on counting start and finish
 * @param finished Set to true when couting is finished, set to false when couting is started
 * @param *context Provided context
 */
typedef void (*PitCallback)(bool finished, void *context);

/**
 * @brief Perform a blocking single-shot PIT measurement
 * @param time Shot time in microseconds
 * @param start Callback on counting start (NULL allowed)
 * @param stop Callback on counting finish (NULL allowed)
 * @param *context Callback context
 * @return ::OK on success, ::BAD_PARAMETER when time is out of valid range
 */
INTERNAL STATUS PitDoSingleShot(uint32_t time, PitCallback start, PitCallback stop, void *context);

/**
 * @}
 */

#endif