#ifndef KERNEL_TSC_H_
#define KERNEL_TSC_H_

#include <stdint.h>
#include "defines.h"

/**
 * @brief Initialize Timestamp Counter and perfrom calibration
 * @return Status code
*/
INTERNAL STATUS TscInit(void);

/**
 * @brief Get raw Timestamp Counter value
 * @return TSC value
*/
INTERNAL uint64_t TscGetRaw(void);

/**
 * @brief Perform TSC calibration with currently best available timer
 * @return Status code
*/
INTERNAL STATUS TscCalibrate(void);

/**
 * @brief Get TSC timestamp in nanoseconds
 * @return Timestamp in nanoseconds or 0 if TSC unavailable
*/
INTERNAL uint64_t TscGet(void);

/**
 * @brief Get TSC timestamp in microseconds
 * @return Timestamp in microseconds or 0 if TSC unavailable
*/
INTERNAL uint64_t TscGetMicros(void);

/**
 * @brief Get TSC timestamp in milliseconds
 * @return Timestamp in milliseconds or 0 if TSC unavailable
*/
INTERNAL uint64_t TscGetMillis(void);

/**
 * @brief Convert nanoseconds to TSC ticks
 * @param time Time in ns
 * @return \a time converted to TSC ticks
*/
INTERNAL uint64_t TscCalculateRaw(uint64_t time);

#endif