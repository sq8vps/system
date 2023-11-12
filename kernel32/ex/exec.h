#ifndef KERNEL_EXEC_H_
#define KERNEL_EXEC_H

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
#include "defines.h"

/**
 * @brief Get executable's required BSS/no-bits section memory size
 * @param *name Executable path
 * @param *size Output size
 * @return Status code
*/
INTERNAL STATUS ExGetExecutableRequiredBssSize(const char *name, uintptr_t *size);

/**
 * @brief Prepare BSS/no-bits sections and update executable header
 * @param *fileStart File start pointer
 * @param *bss Pointer to the space allocated for BSS
 * @return Status code
*/
INTERNAL STATUS ExPrepareExecutableBss(void *fileStart, void *bss);

#endif