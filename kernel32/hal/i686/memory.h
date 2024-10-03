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
 * @brief Create process memory space and map stack on dynamic memory
 * 
 * This function creates page directory for a new process, copies kernel space page tables
 * and allocates ONE page for an initial stack. Then it returns the dynamically mapped stack top.
 * 
 * @param *pdAddress Physical address of newly allocated page directory
 * @param stackAddress Stack top virtual address (in new process memory space)
 * @param **stack Dynamically mapped stack top pointer
 * @return Status code
*/
INTERNAL STATUS I686CreateProcessMemorySpace(PADDRESS *pdAddress, uintptr_t stackAddress, void **stack);

/**
 * @brief Create kernel stack for a thread and map stack on dynamic memory
 * 
 * This function allocates ONE page for an initial stack. Then it returns the dynamically mapped stack top.
 * 
 * @param *parent Parent Task Control Block
 * @param stackAddress Stack top virtual address (in target process memory space)
 * @param **stack Dynamically mapped stack top pointer
 * @return Status code
*/
INTERNAL STATUS I686CreateThreadKernelStack(const struct KeTaskControlBlock *parent, uintptr_t stackAddress, void **stack);

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