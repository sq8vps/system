#ifndef KERNEL32_EXEC_H_
#define KERNEL32_EXEC_H

/**
 * @file exec.h
 * @brief Executable file mainpulation module
 * 
 * Provides routines for file loading and execution.
 * 
 * @defgroup exec Executable and driver module
 * 
*/

#include <stdint.h>
#include "../defines.h"

/**
 * @defgroup kernelSymbols Kernel symbols manipulation routines
 * @ingroup exec
 * @{
*/

/**
 * @brief Get and store kernel symbols from preloaded kernel image
 * @param kernelImage Raw kernel image address
 * @return Error code
 * @attention Full raw image must be loaded to memory (by the bootloader probably)
*/
STATUS Ex_loadKernelSymbols(uintptr_t kernelImage);

/**
 * @brief Resolve kernel symbol
 * @param name Kernel symbol name
 * @return Kernel symbol value
 * @attention Kernel symbol table must be loaded first using Ex_loadKernelSymbols()
*/
uintptr_t Ex_getKernelSymbol(const char *name);

/**
 * @}
*/

#endif