/**
 * @file memory.h
 * @brief x86 memory management module
 * @ingroup i686
 * @note This module implements the universal HAL interface and most of its function are available using kernel API.
*/

#ifndef I686_MEMORY_H_
#define I686_MEMORY_H_

#include <stdint.h>
#include "defines.h"
#include "mm/mm.h"

/**
 * @addtogroup i686_mem x86 memory management
 * @ingroup i686
 * @kinternal
 * 
 * This module is responsible for the actual memory management on x86. This includes
 * memory mapping, synchronizing mapping across CPUs, or maintaining physical memory pools.
 * @{
*/

DRIVER_API

/**
 * @brief i686-specific additional physical memory pools
 */
enum
{
    I686_PHYSICAL_POOL_PCI_DMA = 1, /**< Bottom 4-GiB PCI DMA pool */
    I686_PHYSICAL_POOL_LOWER = 2, /**< Lower-memory (real mode) pool */
    I686_PHYSICAL_POOL_ISA = I686_PHYSICAL_POOL_LOWER, /**< Pool for ISA DMA memory */
};

END_DRIVER_API

struct KeTaskControlBlock;

/**
 * @brief Invalidate TLB for given address on current processor
 * @param address Address to invalidate in TLB
 * @warning This is an internal function.
 */
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