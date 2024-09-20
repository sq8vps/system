#ifndef KERNEL_KSYM_H_
#define KERNEL_KSYM_H_

#include <stdint.h>
#include "defines.h"

/**
 * @defgroup kernelSymbols Kernel symbols manipulation routines
 * @ingroup exec
 * @{
*/

struct Multiboot2InfoHeader;

/**
 * @brief Load and store kernel symbols
 * @param *mb2h Multiboot2 header pointer
 * @return Error code
*/
INTERNAL STATUS ExLoadKernelSymbols(struct Multiboot2InfoHeader *mb2h);

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