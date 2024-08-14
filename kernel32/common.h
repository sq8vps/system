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

EXPORT_API


/**
 * @brief Get string length
 * @param str Input string
 * @return String length (excluding null terminator)
*/
uint32_t CmStrlen(const char *str);


/**
 * @brief Copy string
 * @param strTo Destination string
 * @param strFrom Source string
 * @return Destination string
*/
char* CmStrcpy(char *strTo, const char *strFrom);


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
int CmStrcmp(const char *s1, const char *s2);


/**
 * @brief Compare strings up to a given length
 * @param s1 1st string
 * @param s2 2nd string
 * @param n Length limit
 * @return 0 if identical
*/
int CmStrncmp(const char *s1, const char *s2, int n);


/**
 * @brief Copy memory
 * @param to Destination buffer
 * @param from Source buffer
 * @param n Number of bytes to copy
 * @return Destination buffer
*/
void* CmMemcpy(void *to, const void *from, uintptr_t n);


/**
 * @brief Calculate absolute value of an integer
 * @param x Input integer
 * @return Absolute value of x
*/
uint64_t CmAbs(int64_t x);


/**
 * @brief Fill memory with given value
 * @param *ptr Memory pointer
 * @param c Filler value
 * @param num Byte count
*/
void* CmMemset(void *ptr, int c, uintptr_t num);


/**
 * @brief Compare memory
 * @param *s1 1st memory region
 * @param *s2 2nd memory region
 * @param n Number of bytes to compare
 * @return 0 if equal
*/
int CmMemcmp(const void *s1, const void *s2, uintptr_t n);


/**
 * @brief Check if character is printable
 * @param c Character to check
 * @return 1 if printable, 0 otherwise
*/
int CmIsprint(int c);


/**
 * @brief Concatenate strings
 * @param *dst Destination string
 * @param *src String to be concatenated
 * @return Destination string
*/
char *CmStrcat(char *dst, const char *src);


/**
 * @brief Check if given character is a hexadecimal character
 * @param c Input character
 * @return 1 if is a hexadecimal character, 0 if not
*/
char CmIsxdigit(int c);


/**
 * @brief Check if given character is a digit
 * @param c Input character
 * @return 1 if is a digit, 0 if not
*/
char CmIsdigit(int c);


/**
 * @brief Check if given character is a whitespace
 * @param c Input character
 * @return 1 if is a whitespace character, 0 if not
*/
int CmIsspace(int c);


/**
 * @brief Convert character to lowercase
 * @param c Input character
 * @return Given character converted to lowercase
*/
int CmTolower(int c);


/**
 * @brief Convert character to uppercase
 * @param c Input character
 * @return Given character converted to uppercase
*/
int CmToupper(int c);


/**
 * @brief Calculate natural power of 10
 * @param x Exponent
 * @return 10^x
*/
uint64_t CmPow10(uint16_t x);


/**
 * @brief Extract file name from path
 * @param *path Input path
 * @return Pointer to the first character of file name
*/
char *CmGetFileName(char *path);


/**
 * @brief Allocate table of strings
 * @param countInTable Number of strings in a table
 * @param countToAllocate Number of string to allocate (starting from the first string)
 * @param length Length of each string to be allocated
 * @return Allocated table pointer or NULL on failure
 * @note If either \a countToAllocate or \a length is zero, then no string is allocated.
*/
char** CmAllocateStringTable(uint32_t countInTable, uint32_t countToAllocate, uint32_t length);


/**
 * @brief Free table of strings
 * @param **table Table of strings
 * @param count Number of strings in table
*/
void CmFreeStringTable(char **table, uint32_t count);


/**
 * @brief Check correctness of a file name
 * @param *name File name (not path!)
 * @return True if correct, false if not
*/
bool CmCheckFileName(char *name);



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

END_EXPORT_API

#include "io/disp/print.h"
#define LOG(...) IoPrint(__VA_ARGS__)

/**
 * @}
*/


#endif