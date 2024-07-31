#ifndef I686_MEMORY_H_
#define I686_MEMORY_H_

/**
 * @file memory.h
 * @brief Virtual memory management module
 * 
 * Handles everything connected with virtual memory management.
 * Provides page directory and page table manipulation routines,
 * handles memory mapping and unmapping.
 * 
 * @defgroup i686memory x86 virtual memory management module
 * @ingroup i686
*/

#include <stdint.h>
#include "defines.h"
#include "../cdefines.h"

/**
 * @addtogroup i686memory
 * @{
*/




/**
 * @brief Create and prepare page directory for a process
 * 
 * This function copies page directory kernel space mappings.
 * @return Page directory physical address or 0 on failure
*/
INTERNAL uintptr_t CreateProcessPageDirectory(void);

/**
 * @brief Get current page directory physical address
 * @return Page directory physical address
*/
INTERNAL uintptr_t GetPageDirectoryAddress(void);

/**
 * @brief Switch current page directory
 * @param pageDir New page directory physical address
 * @warning Use exclusively when absolutely neccessary
*/
INTERNAL void SwitchPageDirectory(uintptr_t pageDir);

/**
 * @}
*/

#endif