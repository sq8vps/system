#ifndef I686_TIME_H_
#define I686_TIME_H_

#include "defines.h"

/**
 * @brief Initialize time controller
 * @return Status code
 */
INTERNAL STATUS I686InitTimeController(void);

/**
 * @brief Notify time controller that the LAPIC timer has been started
 */
INTERNAL void I686NotifyLapicTimerStarted(void);

#endif