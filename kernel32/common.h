#ifndef KERNEL_COMMON_H_
#define KERNEL_COMMON_H_

#include <stdint.h>
#include "defines.h"
#include <stdarg.h>
#include <stdbool.h>

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
EXTERN uint64_t CmAbs(int64_t x);

EXPORT
/**
 * @brief Fill memory with given value
 * @param *ptr Memory pointer
 * @param c Filler value
 * @param num Byte count
*/
EXTERN void* CmMemset(void *ptr, int c, uintptr_t num);

EXPORT
/**
 * @brief Compare memory
 * @param *s1 1st memory region
 * @param *s2 2nd memory region
 * @param n Number of bytes to compare
 * @return 0 if equal
*/
EXTERN int CmMemcmp(const void *s1, const void *s2, uintptr_t n);

EXPORT
/**
 * @brief Check if character is printable
 * @param c Character to check
 * @return 1 if printable, 0 otherwise
*/
EXTERN int CmIsprint(int c);

EXPORT
/**
 * @brief Concatenate strings
 * @param *dst Destination string
 * @param *src String to be concatenated
 * @return Destination string
*/
EXTERN char *CmStrcat(char *dst, const char *src);

EXPORT
/**
 * @brief Check if given character is a hexadecimal character
 * @param c Input character
 * @return 1 if is a hexadecimal character, 0 if not
*/
EXTERN char CmIsxdigit(int c);

EXPORT
/**
 * @brief Check if given character is a digit
 * @param c Input character
 * @return 1 if is a digit, 0 if not
*/
EXTERN char CmIsdigit(int c);

EXPORT
/**
 * @brief Check if given character is a whitespace
 * @param c Input character
 * @return 1 if is a whitespace character, 0 if not
*/
EXTERN int CmIsspace(int c);

EXPORT
/**
 * @brief Convert character to lowercase
 * @param c Input character
 * @return Given character converted to lowercase
*/
EXTERN int CmTolower(int c);

EXPORT
/**
 * @brief Convert character to uppercase
 * @param c Input character
 * @return Given character converted to uppercase
*/
EXTERN int CmToupper(int c);

EXPORT
/**
 * @brief Calculate natural power of 10
 * @param x Exponent
 * @return 10^x
*/
EXTERN uint64_t CmPow10(uint16_t x);

EXPORT
/**
 * @brief Extract file name from path
 * @param *path Input path
 * @return Pointer to the first character of file name
*/
EXTERN char *CmGetFileName(char *path);

EXPORT
/**
 * @brief Allocate table of strings
 * @param countInTable Number of strings in a table
 * @param countToAllocate Number of string to allocate (starting from the first string)
 * @param length Length of each string to be allocated
 * @return Allocated table pointer or NULL on failure
 * @note If either \a countToAllocate or \a length is zero, then no string is allocated.
*/
EXTERN char** CmAllocateStringTable(uint32_t countInTable, uint32_t countToAllocate, uint32_t length);

EXPORT
/**
 * @brief Free table of strings
 * @param **table Table of strings
 * @param count Number of strings in table
*/
EXTERN void CmFreeStringTable(char **table, uint32_t count);

EXPORT
/**
 * @brief Check correctness of a file name
 * @param *name File name (not path!)
 * @return True if correct, false if not
*/
EXTERN bool CmCheckFileName(char *name);

EXPORT
#ifdef DEBUG
    #include "io/disp/print.h"
    /**
     * @brief Print message to currently available console
     * @param ... printf-like argument list
     * @return Count of character written
     * @note This is an alias of \a IoPrintDebug()
     * @note This function is automatically excluded in non-debug build
     * @attention This function should not be used except for
     * early boot stage messaging and debugging
    */
    #define PRINT(...) IoPrint(__VA_ARGS__)
    /**
     * @brief Print error message (file name + line numer + additional data) to currently available console
     * @param ... printf-like argument list
     * @return Count of character written
     * @note This is an alias of \a IoPrintDebug() but automatically adding file name and line number
     * @note This function is automatically excluded in non-debug build
     * @attention This function should not be used except for
     * early boot stage messaging and debugging
    */
    #define ERROR(...) IoPrint(__FILE__ ":" STRINGIFY(__LINE__) ": " __VA_ARGS__)
#else
    #define PRINT(...)
    #define ERROR(...)
#endif

#include "io/disp/print.h"
#define LOG(...) IoPrint(__VA_ARGS__)

/**
 * @}
*/


#endif