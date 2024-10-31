#ifndef RTL_STDLIB_H_
#define RTL_STDLIB_H_

#include <stdint.h>
#include "defines.h"

EXPORT_API

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
 * @brief Calculate absolute value of an integer
 * @param x Input integer
 * @return Absolute value of x
*/
uint64_t RtlAbs(int64_t x);

/**
 * @brief Get pointer to file name in given path
 * @param *path Input path
 * @return Pointer to the first character of the file name
*/
const char *RtlGetFileName(const char *path);

/**
 * @brief Check correctness of a file name
 * @param *name File name
 * @return True if correct, false if not
*/
bool RtlCheckFileName(const char *name);

/**
 * @brief Check correctness of a path
 * @param *path Path 
 * @return True if correct, false if not
*/
bool RtlCheckPath(const char *name);

#ifndef DISABLE_KERNEL_STDLIB

/**
 * @brief Calculate absolute value of an integer
 * @param x Input integer
 * @return Absolute value of x
*/
#define abs(x) RtlAbs((x))

/**
 * @brief Generate random number in range 0 to \a RAND_MAX
 * @return Generated random number
 */
#define rand() RtlRandom(0, RAND_MAX)

#endif

END_EXPORT_API

/**
 * @brief Initialize kernel random number generator
 */
INTERNAL void RtlInitializeRandom(void);

#endif