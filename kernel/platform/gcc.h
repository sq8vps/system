/**
 * @file gcc.h
 * @brief GCC-specific definitions and macros
 */

#ifndef KERNEL_PLATFORM_GCC_H_
#define KERNEL_PLATFORM_GCC_H_

#include "export.h"

DRIVER_API

#ifdef __GNUC__

/**
 * @addtogroup defines
 * @{
*/

/**
 * @brief Attribute for packed structures
 * @note Implementation of this attribute is mandatory
*/
#define PACKED __attribute__ ((packed))

/**
 * @brief Mark symbol (function/variable) as internal/hidden
 * @note Implementation of this attribute is recommended
*/
#define INTERNAL __attribute__ ((visibility("hidden")))

/**
 * @brief Mark symbol as weak/overridable
 * @note Implementation of this attribute is mandatory
 */
#define WEAK __attribute__ ((weak))

/**
 * @brief Mark function as frequently called for compiler optimization
 * @note Implementation of this attribute is recommended
 */
#define HOT __attribute__ ((hot))

/**
 * @brief Macro for inline assembly
 * @note Implementation of this attribute is mandatory
*/
#define ASM asm volatile

/**
 * @brief Compiler memory barrier
 * @note Implementation of this attribute is mandatory
 */
#define barrier() ASM("" ::: "memory")

/**
 * @brief Mark branch as extremely likely for compiler optimization
 * @note Implementation of this attribute is recommended
 * @param x Condition to evaluate
 * @return Result of the evalution
 */
#define likely(x) __builtin_expect(!!(x), 1)

/**
 * @brief Mark branch as extremely unlikely for compiler optimization
 * @note Implementation of this attribute is recommended
 * @param x Condition to evaluate
 * @return Result of the evalution
 */
#define unlikely(x) __builtin_expect(!!(x), 0)

/**
 * @brief Mark function as accepting printf-style format
 * @param fmt Number of the format argument
 * @param ellipsis Number of the ellipsis argument
 * @note Implementation of this attribute is recommended
 */
#define PRINTF_LIKE(fmt, ellipsis) __attribute__ ((format(printf, fmt, ellipsis)))

/**
 * @brief Use fastcall convention for a given function
 * @note Implementation of this attribute is mandatory
 */
#define FASTCALL __attribute__ ((fastcall))

/**
 * @brief Mark function as malloc-like
 * @param dealloc Complementary dealloaction function name
 * @note Implementation of this attribute is recommended
 */
#define MALLOC_LIKE(dealloc) __attribute__ ((malloc, malloc(dealloc)))

/**
 * @brief Place function/variable in a given section
 * @param section Target section name string
 * @note Implementation of this attribute is mandatory
 */
#define SECTION(sect) __attribute__ ((section(sect)))

/**
 * @brief Mark function as interrupt service routine
 * @note Implementation of this attribute is mandatory
 */
#define ISR __attribute__ ((interrupt, target("general-regs-only")))

/**
 * @brief Swap halves of x
 * @param x Input number
 * @return \a x with halves swapped
 * @warning This macro evaluates \a x more than once
 * 
 * This macro swaps two halves of a number. For a 16-bit number, the lower byte is swapped with higher byte.
 * For a 32-bit number, the lower 16-bit word is swapped with higher 16-bit word.
 * For a 64-bit number, the lower 32-bit dword is swapped with higher 32-bit dword.
 */
#define BSWAP(x) ((sizeof(long long) == sizeof(x)) ? __builtin_bswap64(x) : \
    ((sizeof(long) == sizeof(x)) ? __builtin_bswap32(x) : __builtin_bswap16(x)))

/**
 * @brief Get nth caller address
 * @param n Caller level: 0 - the caller of the given function
 * @warning This macro is unsafe for \a n > 0
 * @return nth caller address
*/
#define GET_CALLER_ADDRESS(n) (uintptr_t)__builtin_extract_return_addr(__builtin_return_address(n))

/**
 * @brief Relaxed memory ordering
 * @note Implementation of this definition is mandatory
 */
#define ATOMIC_RELAXED __ATOMIC_RELAXED

/**
 * @brief Sequentially consistent memory ordering
 * @note Implementation of this definition is mandatory
 */
#define ATOMIC_SEQ_CST __ATOMIC_SEQ_CST

/**
 * @brief Acquire/release memory ordering
 * @note Implementation of this definition is mandatory
 */
#define ATOMIC_ACQ_REL __ATOMIC_ACQ_REL

/**
 * @brief Acquire memory ordering
 * @note Implementation of this definition is mandatory
 */
#define ATOMIC_ACQUIRE __ATOMIC_ACQUIRE

/**
 * @brief Release memory ordering
 * @note Implementation of this definition is mandatory
 */
#define ATOMIC_RELEASE __ATOMIC_RELEASE

/**
 * @brief Atomically load value from \a ptr
 * @param ptr Pointer to a variable
 * @param order Memory ordering
 * @return Value loaded from \a ptr
 * @note Implementation of this macro is mandatory
 */
#define ATOMIC_LOAD(ptr, order) __atomic_load_n(ptr, order)

/**
 * @brief Atomically store value to \a ptr
 * @param ptr Pointer to a variable
 * @param val Value to store
 * @param order Memory ordering
 * @note Implementation of this macro is mandatory
 */
#define ATOMIC_STORE(ptr, val, order) __atomic_store_n(ptr, val, order)

/**
 * @brief Atomically fetch \a ptr and then add \a val to \a ptr
 * @param ptr Pointer to a variable
 * @param val Value to add to \a ptr
 * @param order Memory ordering
 * @return Value from \a ptr before add
 * @note Implementation of this macro is mandatory
 */
#define ATOMIC_FETCH_ADD(ptr, val, order) __atomic_fetch_add(ptr, val, order)

/**
 * @brief Atomically add \a val to \a ptr and then fetch \a ptr
 * @param ptr Pointer to a variable
 * @param val Value to subtract from \a ptr
 * @param order Memory ordering
 * @return Value from \a ptr after add
 * @note Implementation of this macro is mandatory
 */
#define ATOMIC_ADD_FETCH(ptr, val, order) __atomic_add_fetch(ptr, val, order)

/**
 * @brief Atomically subtract \a val from \a ptr and then fetch \a ptr
 * @param ptr Pointer to a variable
 * @param val Value to subtract from \a ptr
 * @param order Memory ordering
 * @return Value from \a ptr after subtraction
 * @note Implementation of this macro is mandatory
 */
#define ATOMIC_SUB_FETCH(ptr, val, order) __atomic_sub_fetch(ptr, val, order)

/**
 * @brief Atomically fetch \a ptr and then add \a val to \a ptr
 * @param ptr Pointer to a variable
 * @param val Value to subtract from \a ptr
 * @param order Memory ordering
 * @return Value from \a ptr before subtraction
 * @note Implementation of this macro is mandatory
 */
#define ATOMIC_FETCH_SUB(ptr, val, order) __atomic_fetch_sub(ptr, val, order)

/**
 * @brief Atomically fetch \a ptr and then binary OR \a ptr with \a val
 * @param ptr Pointer to a variable
 * @param val Value to OR with \a ptr
 * @param order Memory ordering
 * @return Value from \a ptr before ORing
 * @note Implementation of this macro is mandatory
 */
#define ATOMIC_FETCH_OR(ptr, val, order) __atomic_fetch_or(ptr, val, order)

/**
 * @brief Atomically fetch \a ptr and then binary AND \a ptr with \a val
 * @param ptr Pointer to a variable
 * @param val Value to AND with \a ptr
 * @param order Memory ordering
 * @return Value from \a ptr before ANDing
 * @note Implementation of this macro is mandatory
 */
#define ATOMIC_FETCH_AND(ptr, val, order) __atomic_fetch_and(ptr, val, order)

/**
 * @brief Atomically fetch \a ptr and then move \a val to \a ptr
 * @param ptr Pointer to a variable
 * @param val Value to move to \a ptr
 * @param order Memory ordering
 * @return Value from \a ptr before moving
 * @note Implementation of this macro is mandatory
 */
#define ATOMIC_EXCHANGE(ptr, val, order) __atomic_exchange_n(ptr, val, order)

/**
 * @brief Count 1-bits in \a x
 * @param x Input number
 * @return Number of 1-bits in \a x
 * @note Implementation of this macro is mandatory
 * @note This should be removed and replaced by \c stdbit.h, but it seems to be not available in freestanding environment
 */
#define stdc_count_ones(x) __builtin_stdc_count_ones(x)

/**
 * @brief Count trailing 0-bits in \a x starting from LSBit
 * @param x Input number
 * @return Number of trailing 0-bits in \a x
 * @warning If \a x is 0, the results is undefined
 * @note Implementation of this macro is mandatory
 * @note This should be removed and replaced by \c stdbit.h, but it seems to be not available in freestanding environment
 */
#define stdc_trailing_zeros(x) __builtin_stdc_trailing_zeros(x)

/**
 * @brief Count leading 0-bits in \a x starting from MSBit
 * @param x Input number
 * @return Number of leading 0-bits in \a x
 * @warning If \a x is 0, the results is undefined
 * @note Implementation of this macro is mandatory
 * @note This should be removed and replaced by \c stdbit.h, but it seems to be not available in freestanding environment
 */
#define stdc_leading_zeros(x) __builtin_stdc_leading_zeros(x)

/**
 * @brief Check whether \a x has exactly one 1-bit
 * @param x Input number
 * @return True if \a x has exactly one 1-bit
 * @note Implementation of this macro is mandatory
 * @note This should be removed and replaced by \c stdbit.h, but it seems to be not available in freestanding environment
 */
#define stdc_has_single_bit(x) __builtin_stdc_has_single_bit(x)

/**
 * @brief Find the largest integral power of 2 not greater than the given value
 * @param x Input number
 * @return Largest intergral power of 2 not greater than \a x
 * @note This should be removed and replaced by \c stdbit.h, but it seems to be not available in freestanding environment
 */
#define stdc_bit_floor(x) __builtin_stdc_bit_floor(x)

/**
 * @}
 */

#endif

END_DRIVER_API

#endif