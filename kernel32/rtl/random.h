#ifndef RTL_RANDOM_H_
#define RTL_RANDOM_H_

#include <stdint.h>

#define RAND_MAX 32767

/**
 * @brief Generate random number in given range
 * @param min Lower limit
 * @param max Upper limit
 * @return Generated random number
 * @warning This generator is not cryptographically safe
 * @attention This generator is not seeded properly before \a RtlInintializeRandom() is called
 */
int32_t RtlRandom(int32_t min, int32_t max);

/**
 * @brief Generate random number in range 0 to \a RAND_MAX
 * @return Generated random number
 */
#define rand() RtlRandom(0, RAND_MAX)

/**
 * @brief Initialize kernel random number generator
 */
void RtlInitializeRandom(void);

#endif