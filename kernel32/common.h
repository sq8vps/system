#ifndef KERNEL32_COMMON_H_
#define KERNEL32_COMMON_H_

#include <stdint.h>

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

/**
 * @brief General RGBA color structure
*/
typedef struct
{
    float r;
    float g;
    float b;
    float alpha;
} Cm_RGBA_t;

/**
 * @brief Get string length
 * @param str Input string
 * @return String length (excluding null terminator)
*/
uint32_t Cm_strlen(const char *str);

/**
 * @brief Copy string
 * @param strTo Destination string
 * @param strFrom Source string
 * @return Destination string
*/
char* Cm_strcpy(char *strTo, const char *strFrom);

/**
 * @brief Copy at most n bytes of a string
 * @param strTo Destination string
 * @param strFrom Source string
 * @param n Character limit (NULL terminator excluded)
 * @return Destination string
*/
char* CmStrncpy(char *strTo, const char *strFrom, uintptr_t n);

/**
 * @brief Compare strings
 * @param s1 1st string
 * @param s2 2nd string
 * @return 0 if identical
*/
int Cm_strcmp(const char *s1, const char *s2);

/**
 * @brief Copy memory
 * @param to Destination buffer
 * @param from Source buffer
 * @param n Number of bytes to copy
 * @return Destination buffer
*/
void* Cm_memcpy(void *to, const void *from, uintptr_t n);

/**
 * @brief Calculate absolute value of an integer
 * @param x Input integer
 * @return Absolute value of x
*/
uint32_t Cm_abs(int32_t x);

/**
 * @brief Fill memory with given value
 * @param *ptr Memory pointer
 * @param c Filler value
 * @param num Byte count
*/
void* Cm_memset(void *ptr, int c, uintptr_t num);

/**
 * @}
*/


#endif