/**
 * @file platform.h
 * @brief Compiler-specific definitions wrapper
 */

#ifndef KERNEL_PLATFORM_PLATFORM_H_
#define KERNEL_PLATFORM_PLATFORM_H_

#include "export.h"

DRIVER_API

#ifdef __GNUC__

#include "gcc.h"

#else

#error Unsupported compiler

#endif

#ifndef INTERNAL
/**
 * @brief Mark symbol (function/variable) as internal/hidden
 * @note Implementation of this attribute is recommended
*/
#define INTERNAL
#endif

#ifndef HOT
/**
 * @brief Mark function as frequently called for compiler optimization
 * @note Implementation of this attribute is recommended
 */
#define HOT
#endif

#ifndef likely
/**
 * @brief Mark branch as extremely likely for compiler optimization
 * @note Implementation of this attribute is recommended
 * @param x Condition to evaluate
 * @return Result of the evalution
 */
#define likely(x) (x)
#endif

#ifndef unlikely
/**
 * @brief Mark branch as extremely unlikely for compiler optimization
 * @note Implementation of this attribute is recommended
 * @param x Condition to evaluate
 * @return Result of the evalution
 */
#define unlikely(x) (x)
#endif

#ifndef PRINTF_LIKE
/**
 * @brief Mark function as accepting printf-style format
 * @param fmt Number of the format argument
 * @param ellipsis Number of the ellipsis argument
 * @note Implementation of this attribute is recommended
 */
#define PRINTF_LIKE(fmt, ellipsis)
#endif

#ifndef MALLOC_LIKE
/**
 * @brief Mark function as malloc-like
 * @param dealloc Complementary dealloaction function name
 * @note Implementation of this attribute is recommended
 */
#define MALLOC_LIKE(dealloc)
#endif

END_DRIVER_API

#endif