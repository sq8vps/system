/**
 * @file ksym.h
 * @brief Kernel symbol handling
 * @ingroup exec
 */

#ifndef KERNEL_KSYM_H_
#define KERNEL_KSYM_H_

#include <stdint.h>
#include "defines.h"

/**
 * @addtogroup kernel_symbols Kernel symbols handling routines
 * @ingroup exec
 * @{
*/

/**
 * @brief Load and store kernel symbols
 * @param *bootArgs Bootloader data
 * @kinternal
 * @return Error code
*/
INTERNAL STATUS ExLoadKernelSymbols(const void *bootArgs);

DRIVER_API

/**
 * @brief Resolve kernel symbol
 * @param name Kernel symbol name
 * @return Kernel symbol value
 * @attention Kernel symbol table must be loaded first using ExLoadKernelSymbols()
*/
uintptr_t ExGetKernelSymbol(const char *name);

END_DRIVER_API

/**
 * @}
*/


#endif