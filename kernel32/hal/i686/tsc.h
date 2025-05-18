#ifndef KERNEL_TSC_H_
#define KERNEL_TSC_H_

#include <stdint.h>
#include "defines.h"

/**
 * @brief Initialize Timestamp Counter and perfrom calibration on bootstrap CPU
 * @return Status code
*/
INTERNAL STATUS TscInit(void);

/**
 * @brief Initialize Timestamp Counter on Application CPUs in a SMP system
 * @return Status code
 */
INTERNAL STATUS TscInitForSmp(void);

/**
 * @brief Update TSC state
 */
INTERNAL void TscUpdate(void);

/**
 * @brief Get raw Timestamp Counter value for current CPU
 * @param *context Unused, set to NULL
 * @return TSC value
*/
INTERNAL uint64_t TscGetRaw(void *context);

/**
 * @brief Perform TSC calibration with currently best available timer
 * @return Status code
*/
INTERNAL STATUS TscCalibrate(void);

/**
 * @brief Convert nanoseconds to TSC ticks
 * @param time Time in ns
 * @return \a time converted to TSC ticks
 * @attention Mind the overflows of 64 bit multiplication!
 * @note This function should be used to calculate ticks for small (delta) times
*/
INTERNAL uint64_t TscCalculateRaw(uint32_t time);

#endif