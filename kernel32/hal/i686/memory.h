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

struct KeTaskControlBlock;

/**
 * @brief Create process memory space and map stack on dynamic memory
 * 
 * This function creates page directory for a new process, copies kernel space page tables
 * and allocates ONE page for a initial stack. Then it returns the dynamically mapped stack top.
 * 
 * @param *pdAddress Physical address of newly allocated page directory
 * @param stackAddress Stack top virtual address (in new process memory space)
 * @param **stack Dynamically mapped stack top pointer
 * @return Status code
*/
INTERNAL STATUS I686CreateProcessMemorySpace(PADDRESS *pdAddress, uintptr_t stackAddress, void **stack);

/**
 * @brief Get current page directory physical address
 * @return Page directory physical address
*/
INTERNAL uintptr_t I686GetPageDirectoryAddress(void);

/**
 * @brief Initialize virtual memory allocator
 */
INTERNAL void I686InitVirtualAllocator(void);

/**
 * @}
*/

#endif