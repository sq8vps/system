#ifndef KERNEL_COMMON_H_
#define KERNEL_COMMON_H_

#include <stdint.h>
#include "defines.h"

/**
 * @file common.h
 * @brief Common (independent) kernel functions
 * 
 * Provides a set of common kernel mode functions.
 * 
 * @defgroup common Common kernel functions
*/

/**
 * @ingroup common
 * @{
*/

EXPORT
/**
 * @brief Get string length
 * @param str Input string
 * @return String length (excluding null terminator)
*/
EXTERN uint32_t CmStrlen(const char *str);

EXPORT
/**
 * @brief Copy string
 * @param strTo Destination string
 * @param strFrom Source string
 * @return Destination string
*/
EXTERN char* CmStrcpy(char *strTo, const char *strFrom);

EXPORT
/**
 * @brief Copy at most n bytes of a string
 * @param strTo Destination string
 * @param strFrom Source string
 * @param n Character limit (NULL terminator excluded)
 * @return Destination string
*/
EXTERN char* CmStrncpy(char *strTo, const char *strFrom, uintptr_t n);

EXPORT
/**
 * @brief Compare strings
 * @param s1 1st string
 * @param s2 2nd string
 * @return 0 if identical
*/
EXTERN int CmStrcmp(const char *s1, const char *s2);

EXPORT
/**
 * @brief Compare strings up to a given length
 * @param s1 1st string
 * @param s2 2nd string
 * @param n Length limit
 * @return 0 if identical
*/
EXTERN int CmStrncmp(const char *s1, const char *s2, int n);

EXPORT
/**
 * @brief Copy memory
 * @param to Destination buffer
 * @param from Source buffer
 * @param n Number of bytes to copy
 * @return Destination buffer
*/
EXTERN void* CmMemcpy(void *to, const void *from, uintptr_t n);

EXPORT
/**
 * @brief Calculate absolute value of an integer
 * @param x Input integer
 * @return Absolute value of x
*/
EXTERN uint32_t CmAbs(int32_t x);

EXPORT
/**
 * @brief Fill memory with given value
 * @param *ptr Memory pointer
 * @param c Filler value
 * @param num Byte count
*/
EXTERN void* CmMemset(void *ptr, int c, uintptr_t num);

EXPORT
EXTERN int CmPrintf(const char *format, ...);

/**
 * @}
*/


#endif