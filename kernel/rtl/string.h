/**
 * @file string.h
 * @brief Kernel \c string.h implementation
 * @ingroup rtl_string
 */

#ifndef RTL_STRING_H_
#define RTL_STRING_H_

#include "defines.h"
#include <stdint.h>

/**
 * @addtogroup rtl_string Kernel \c string.h implementation
 * @ingroup rtl
 * @note To disable aliasing RTL-specific names with C-standard names, define \c DISABLE_KERNEL_STDLIB before including this header.
 * @warning This implementation does not adhere to any C standard.
 * @{
 */


DRIVER_API

#define RTL_UNICODE_ZWJ (uint32_t)0x200D /**< Unicode Zero-width joiner value */

/**
 * @brief Get string length
 * @param str Input string
 * @return String length (excluding null terminator)
*/
size_t RtlStrlen(const char *str);

/**
 * @brief Get user string length
 * @param str Input string
 * @return String length (excluding null terminator) or -1 if memory is inaccessible
*/
size_t RtlStrlenUser(const char *str);

/**
 * @brief Copy string
 * @param strTo Destination string
 * @param strFrom Source string
 * @return Destination string
*/
char* RtlStrcpy(char *restrict strTo, const char *restrict strFrom);

/**
 * @brief Copy at most n bytes of a string
 * @param strTo Destination string
 * @param strFrom Source string
 * @param n Character limit (NULL terminator excluded)
 * @return Destination string
*/
char* RtlStrncpy(char *restrict strTo, const char *restrict strFrom, size_t n);

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
int RtlStrncmp(const char *s1, const char *s2, size_t n);

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
int RtlStrcasencmp(const char *s1, const char *s2, size_t n);

/**
 * @brief Copy memory
 * @param to Destination buffer
 * @param from Source buffer
 * @param n Number of bytes to copy
 * @return Destination buffer
*/
void* RtlMemcpy(void *restrict to, const void *restrict from, size_t n);

/**
 * @brief Copy volatile memory
 * @param to Destination buffer
 * @param from Source buffer
 * @param n Number of bytes to copy
 * @return Destination buffer
*/
volatile void* RtlMemcpyV(volatile void *restrict to, volatile const void *restrict from, size_t n);

/**
 * @brief Move memory
 * @param to Destination buffer
 * @param from Source buffer
 * @param n Number of bytes to move
 * @return Destination buffer
*/
void *RtlMemmove(void *to, const void *from, size_t n);

/**
 * @brief Fill memory with given value
 * @param *ptr Memory pointer
 * @param c Filler value
 * @param num Byte count
*/
void* RtlMemset(void *ptr, int c, size_t num);

/**
 * @brief Fill volatile memory with given value
 * @param *ptr Memory pointer
 * @param c Filler value
 * @param num Byte count
*/
volatile void* RtlMemsetV(volatile void *ptr, int c, size_t num);

/**
 * @brief Compare memory
 * @param *s1 1st memory region
 * @param *s2 2nd memory region
 * @param n Number of bytes to compare
 * @return 0 if equal
*/
int RtlMemcmp(const void *s1, const void *s2, size_t n);

/**
 * @brief Concatenate strings
 * @param *dst Destination string
 * @param *src String to be concatenated
 * @return Destination string
*/
char *RtlStrcat(char *restrict dst, const char *restrict src);

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

/**
 * @brief Get UTF-8 byte count based on the first byte value
 * @param c First UTF-8/ASCII character
 * @return Number of bytes in UTF-8 character
 * @return Always 1 if ASCII character
 */
size_t RtlUtf8ByteCount(char c);

/**
 * @brief Check whether an Unicode codepoint is combining
 * @param code Codepoint to be checked
 * @return True if \a code is combining, false otherwise
 */
bool RtlUnicodeIsCombining(uint32_t code);

/**
 * @brief Check whether an Unicode codepoint is a variation selector
 * @param code Codepoint to be checked
 * @return True if \a code is a variation selector, false otherwise
 */
bool RtlUnicodeIsVariationSelector(uint32_t code);

/**
 * @brief Check whether an Unicode codepoint is extending
 * @param code Codepoint to be checked
 * @return True if \a code is extending, false otherwise
 */
bool RtlUnicodeIsExtending(uint32_t code);

/**
 * @brief Check whether an Unicode character starts a new grapheme cluster
 * @param previous Last Unicode codepoint
 * @param current Current Unicode codepoint
 * @return True if \a current starts a new grapheme cluster, and \a previous is a part of a previous grapheme cluster.
 */
bool RtlUnicodeIsGraphemeClusterStart(uint32_t previous, uint32_t current);

/**
 * @brief Get length of a Unicode grapheme cluster
 * @param *code Pointer to a codepoint array
 * @param size Length of \a *code array (number of elements)
 * @return Number of elements in the grapheme cluster
 */
size_t RtlUnicodeGraphemeClusterLength(const uint32_t *code, size_t size);

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


END_DRIVER_API

/**
 * @}
 */

#endif