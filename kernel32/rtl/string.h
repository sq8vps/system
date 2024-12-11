#ifndef RTL_STRING_H_
#define RTL_STRING_H_

#include "defines.h"
#include <stdint.h>

EXPORT_API

/**
 * @brief Get string length
 * @param str Input string
 * @return String length (excluding null terminator)
*/
uint32_t RtlStrlen(const char *str);

/**
 * @brief Copy string
 * @param strTo Destination string
 * @param strFrom Source string
 * @return Destination string
*/
char* RtlStrcpy(char *strTo, const char *strFrom);

/**
 * @brief Copy at most n bytes of a string
 * @param strTo Destination string
 * @param strFrom Source string
 * @param n Character limit (NULL terminator excluded)
 * @return Destination string
*/
char* RtlStrncpy(char *strTo, const char *strFrom, uintptr_t n);

/**
 * @brief Compare strings
 * @param s1 1st string
 * @param s2 2nd string
 * @return 0 if identical
*/
int RtlStrcmp(const char *s1, const char *s2);

/**
 * @brief Compare strings up to a given length
 * @param s1 1st string
 * @param s2 2nd string
 * @param n Length limit
 * @return 0 if identical
*/
int RtlStrncmp(const char *s1, const char *s2, int n);

/**
 * @brief Compare strings - case-insensitive
 * @param s1 1st string
 * @param s2 2nd string
 * @return 0 if identical
*/
int RtlStrcasecmp(const char *s1, const char *s2);

/**
 * @brief Compare strings up to a given length - case-insensitive
 * @param s1 1st string
 * @param s2 2nd string
 * @param n Length limit
 * @return 0 if identical
*/
int RtlStrcasencmp(const char *s1, const char *s2, int n);

/**
 * @brief Copy memory
 * @param to Destination buffer
 * @param from Source buffer
 * @param n Number of bytes to copy
 * @return Destination buffer
*/
void* RtlMemcpy(void *to, const void *from, uintptr_t n);

/**
 * @brief Copy volatile memory
 * @param to Destination buffer
 * @param from Source buffer
 * @param n Number of bytes to copy
 * @return Destination buffer
*/
volatile void* RtlMemcpyV(volatile void *to, volatile const void *from, uintptr_t n);

/**
 * @brief Fill memory with given value
 * @param *ptr Memory pointer
 * @param c Filler value
 * @param num Byte count
*/
void* RtlMemset(void *ptr, int c, uintptr_t num);

/**
 * @brief Fill volatile memory with given value
 * @param *ptr Memory pointer
 * @param c Filler value
 * @param num Byte count
*/
volatile void* RtlMemsetV(volatile void *ptr, int c, uintptr_t num);

/**
 * @brief Compare memory
 * @param *s1 1st memory region
 * @param *s2 2nd memory region
 * @param n Number of bytes to compare
 * @return 0 if equal
*/
int RtlMemcmp(const void *s1, const void *s2, uintptr_t n);

/**
 * @brief Concatenate strings
 * @param *dst Destination string
 * @param *src String to be concatenated
 * @return Destination string
*/
char *RtlStrcat(char *dst, const char *src);

/**
 * @brief Convert octal number string to uint32_t
 * @param *octal String containing octal number
 * @return Number converted to uint32_t
 */
uint32_t RtlOctalToU32(const char *octal);

/**
 * @brief Allocate table of strings
 * @param countInTable Number of strings in a table
 * @param countToAllocate Number of string to allocate (starting from the first string)
 * @param length Length of each string to be allocated
 * @return Allocated table pointer or NULL on failure
 * @note If either \a countToAllocate or \a length is zero, then no string is allocated.
*/
char** RtlAllocateStringTable(uint32_t countInTable, uint32_t countToAllocate, uint32_t length);

/**
 * @brief Free table of strings
 * @param **table Table of strings
 * @param count Number of strings in table
*/
void RtlFreeStringTable(char **table, uint32_t count);

#ifndef DISABLE_KERNEL_STDLIB

/**
 * @brief Get string length
 * @param str Input string
 * @return String length (excluding null terminator)
*/
#define strlen(str) RtlStrlen((str))

/**
 * @brief Copy string
 * @param strTo Destination string
 * @param strFrom Source string
 * @return Destination string
*/
#define strcpy(strTo, strFrom) RtlStrcpy((strTo), (strFrom))

/**
 * @brief Copy at most n bytes of a string
 * @param strTo Destination string
 * @param strFrom Source string
 * @param n Character limit (NULL terminator excluded)
 * @return Destination string
*/
#define strncpy(strTo, strFrom, n) RtlStrncpy((strTo), (strFrom), (n))

/**
 * @brief Compare strings
 * @param s1 1st string
 * @param s2 2nd string
 * @return 0 if identical
*/
#define strcmp(s1, s2) RtlStrcmp((s1), (s2))

/**
 * @brief Compare strings up to a given length
 * @param s1 1st string
 * @param s2 2nd string
 * @param n Length limit
 * @return 0 if identical
*/
#define strncmp(s1, s2, n) RtlStrncmp((s1), (s2), (n))

/**
 * @brief Compare strings - case-insensitive
 * @param s1 1st string
 * @param s2 2nd string
 * @return 0 if identical
*/
#define strcasecmp(s1, s2) RtlStrcasecmp((s1), (s2))

/**
 * @brief Compare strings up to a given length - case-insensitive
 * @param s1 1st string
 * @param s2 2nd string
 * @param n Length limit
 * @return 0 if identical
*/
#define strcasencmp(s1, s2, n) RtlStrcasencmp((s1), (s2), (n))

/**
 * @brief Copy memory
 * @param to Destination buffer
 * @param from Source buffer
 * @param n Number of bytes to copy
 * @return Destination buffer
*/
#define memcpy(to, from, n) RtlMemcpy((to), (from), (n))

/**
 * @brief Fill memory with given value
 * @param *ptr Memory pointer
 * @param c Filler value
 * @param num Byte count
*/
#define memset(ptr, c, num) RtlMemset((ptr), (c), (num))

/**
 * @brief Compare memory
 * @param *s1 1st memory region
 * @param *s2 2nd memory region
 * @param n Number of bytes to compare
 * @return 0 if equal
*/
#define memcmp(s1, s2, n) RtlMemcmp((s1), (s2), (n))

/**
 * @brief Concatenate strings
 * @param *dst Destination string
 * @param *src String to be concatenated
 * @return Destination string
*/
#define strcat(dst, src) RtlStrcat((dst), (src))

#endif


END_EXPORT_API

#endif