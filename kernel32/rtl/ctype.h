#ifndef RTL_CTYPE_H_
#define RTL_CTYPE_H_

#include "defines.h"

EXPORT_API

/**
 * @brief Check if character is printable
 * @param c Character to check
 * @return 1 if printable, 0 otherwise
*/
int RtlIsprint(int c);

/**
 * @brief Check if given character is a hexadecimal character
 * @param c Input character
 * @return 1 if is a hexadecimal character, 0 if not
*/
char RtlIsxdigit(int c);

/**
 * @brief Check if given character is a digit
 * @param c Input character
 * @return 1 if is a digit, 0 if not
*/
char RtlIsdigit(int c);

/**
 * @brief Check if given character is a whitespace
 * @param c Input character
 * @return 1 if is a whitespace character, 0 if not
*/
int RtlIsspace(int c);

/**
 * @brief Convert character to lowercase
 * @param c Input character
 * @return Given character converted to lowercase
*/
int RtlTolower(int c);

/**
 * @brief Convert character to uppercase
 * @param c Input character
 * @return Given character converted to uppercase
*/
int RtlToupper(int c);

#ifndef DISABLE_KERNEL_STDLIB

/**
 * @brief Check if given character is a hexadecimal character
 * @param c Input character
 * @return 1 if is a hexadecimal character, 0 if not
*/
#define isxdigit(c) RtlIsxdigit((c))
/**
 * @brief Check if given character is a digit
 * @param c Input character
 * @return 1 if is a digit, 0 if not
*/
#define isdigit(c) RtlIsdigit((c))

/**
 * @brief Check if given character is a whitespace
 * @param c Input character
 * @return 1 if is a whitespace character, 0 if not
*/
#define isspace(c) RtlIsspace((c))

/**
 * @brief Convert character to lowercase
 * @param c Input character
 * @return Given character converted to lowercase
*/
#define tolower(c) RtlTolower((c))

/**
 * @brief Convert character to uppercase
 * @param c Input character
 * @return Given character converted to uppercase
*/
#define toupper(c) RtlToupper((c))

#endif

END_EXPORT_API

#endif