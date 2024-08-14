//This header file is generated automatically
#ifndef EXPORTED___API__HAL_TIME_H_
#define EXPORTED___API__HAL_TIME_H_

#ifdef __cplusplus
extern "C" 
{
#endif

#include "defines.h"
#include <stdint.h>

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


#ifdef __cplusplus
}
#endif

#endif