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
 * @brief Prepare PIT (channel 2) in single shot mode
 * @param time Time in microseconds
*/
void PitOneShotInit(uint32_t time);

/**
 * @brief Start PIT one-shot timer
*/
void PitOneShotStart(void);

/**
 * @brief Wait for one-shot timer end
*/
void PitOneShotWait(void);

#endif