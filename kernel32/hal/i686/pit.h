#ifndef KERNEL_PIT_H_
#define KERNEL_PIT_H_

#include <stdint.h>
#include "defines.h"


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
 * @brief Prepare PIT (channel 2) in single shot mode
 * @param time Time in microseconds
*/
INTERNAL void PitOneShotInit(uint32_t time);

/**
 * @brief Start PIT one-shot timer
*/
INTERNAL void PitOneShotStart(void);

/**
 * @brief Wait for one-shot timer end
*/
INTERNAL void PitOneShotWait(void);

#endif