#ifndef KERNEL_KSYM_H_
#define KERNEL_KSYM_H_

#include <stdint.h>
#include "defines.h"

/**
 * @defgroup kernelSymbols Kernel symbols manipulation routines
 * @ingroup exec
 * @{
*/

/**
 * @brief Get and store kernel symbols from kernel image
 * @param *path Kernel ELF image path
 * @return Error code
*/
INTERNAL STATUS ExLoadKernelSymbols(char *path);

EXPORT_API

/**
 * @brief Resolve kernel symbol
 * @param name Kernel symbol name
 * @return Kernel symbol value
 * @attention Kernel symbol table must be loaded first using ExLoadKernelSymbols()
*/
uintptr_t ExGetKernelSymbol(const char *name);

END_EXPORT_API

/**
 * @}
*/


#endif