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
#include "mm/mm.h"

/**
 * @addtogroup i686memory
 * @{
*/

/**
 * @brief i686-specific additional physical memory pools
 */
enum
{
    I686_PHYSICAL_POOL_LOWER = 1, /**< Lower-memory pool */
    I686_PHYSICAL_POOL_ISA = I686_PHYSICAL_POOL_LOWER, /**< Pool for ISA DMA memory */
};

struct KeTaskControlBlock;

#define I686_INVALIDATE_TLB(address) ASM("invlpg [%0]" : : "r" (address) : "memory")

/**
 * @brief Get page flags for lazy TLB invalidation in page fault handler
 * @param address Faulting page address
 * @return Page flags
 */
INTERNAL MmMemoryFlags I686GetPageFlagsFromPageFault(uintptr_t address);

/**
 * @brief Create new memory space
 * 
 * This function creates new page directory and copies kernel space page tables.
 * 
 * @return Page directory physical address or 0 on memory allocation failure
*/
INTERNAL PADDRESS I686CreateNewMemorySpace(void);

/**
 * @brief Destroy memory space
 * @param pdAddress Page directory physical address
 */
INTERNAL void I686DestroyMemorySpace(PADDRESS pdAddress);

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