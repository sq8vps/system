#ifndef KERNEL_PIT_H_
#define KERNEL_PIT_H_

#include <stdint.h>
#include "defines.h"


/**
 * @brief Initialize Programmable Interval Timer
*/
void PitInit(void);

/**
 * @brief Set PIT (channel 0) interval
 * @param interval Interval in milliseconds
*/
void PitSetInterval(uint32_t interval);

/**
 * @brief Prepare PIT (channel 2) for time measurement
 * @param time PIT interval in microseconds
*/
void PitOneShotInit(uint32_t time);

/**
 * @brief Start PIT measurement (blocking!)
 * @param *currentCounter External counter pointer
 * @param *initialCounter External initial counter value pointer
 * @param initial Initial counter value
 * @return Difference between current and initial counter value
*/
uint32_t PitOneShotMeasure(uint32_t *counter, uint32_t *initialCounter,uint32_t initial);

#endif