/**
 * @file ctype.h
 * @brief Kernel \c ctype.h implementation
 * @ingroup rtl_ctype
 */

#ifndef RTL_CTYPE_H_
#define RTL_CTYPE_H_

#include "defines.h"

/**
 * @addtogroup rtl_ctype Kernel \c ctype.h implementation
 * @ingroup rtl
 * @note To disable aliasing RTL-specific names with C-standard names, define \c DISABLE_KERNEL_STDLIB before including this header.
 * @warning This implementation does not adhere to any C standard.
 * @{
 */

DRIVER_API

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

/**
 * @}
 */

END_DRIVER_API

#endif