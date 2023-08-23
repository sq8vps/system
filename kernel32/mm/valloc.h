#ifndef KERNEL_VALLOC_H_
#define KERNEL_VALLOC_H_

/**
 * @file valloc.h
 * @brief Virtual memory management module
 * 
 * Handles everything connected with virtual memory management.
 * Provides page directory and page table manipulation routines,
 * handles memory mapping and unmapping.
 * 
 * @ingroup mm
*/

#include <stdint.h>
#include "defines.h"
#include "../cdefines.h"

/**
 * @defgroup vMem Virtual memory module
 * @ingroup mm
 * @{
*/

EXPORT
typedef uint32_t MmPageDirectoryEntry_t;
typedef uint32_t MmPageTableEntry_t;
typedef uint16_t MmPagingFlags_t;

EXPORT
/**
 * @brief Flags for page directories and page table entries
*/
#define MM_PAGE_FLAG_PRESENT 1
#define MM_PAGE_FLAG_WRITABLE 2
#define MM_PAGE_FLAG_USER 4
#define MM_PAGE_FLAG_PWT 8
#define MM_PAGE_FLAG_PCD 16


/**
 * @brief Initialize virtual allocator
*/
void MmInitVirtualAllocator(void);

EXPORT
/**
 * @brief Get page table entry flags
 * @param vAddress Input virtual address
 * @param *flags Output flags
 * @return Status code
*/
EXTERN STATUS MmGetPageFlags(uintptr_t vAddress, MmPagingFlags_t *flags);

EXPORT
/**
 * @brief Get current process physical address corresponding to the virtual address
 * @param vAddress Input virtual address
 * @param *pAddress Output physical address
 * @return Error code
 */
EXTERN STATUS MmGetPhysicalPageAddress(uintptr_t vAddress, uintptr_t *pAddress);

EXPORT
/**
 * @brief Map current process memory
 * @param vAddress Address to map the memory to
 * @param pAddress Physical memory address
 * @param flags Page flags
 * @return Error code
 * @attention This function does not allocate physical memory.
*/
EXTERN STATUS MmMapMemory(uintptr_t vAddress, uintptr_t pAddress, MmPagingFlags_t flags);

EXPORT
/**
 * @brief Map multiple pages of current process memory
 * @param vAddress Address to map the memory to
 * @param pAddress Physical memory address
 * @param size Memory size in bytes
 * @param flags Page flags
 * @return Error code
 * @attention This function does not allocate physical memory.
*/
EXTERN STATUS MmMapMemoryEx(uintptr_t vAddress, uintptr_t pAddress, uintptr_t size, MmPagingFlags_t flags);

EXPORT
/**
 * @brief Unmap current process memory
 * @param vAddress Address to map the memory to
 * @return Error code
 * @attention This function does not free physical memory.
*/
EXTERN STATUS MmUnmapMemory(uintptr_t vAddress);

EXPORT
/**
 * @brief Unmap multiple pages of current process memory
 * @param vAddress Address to unmap
 * @param size Memory size in bytes
 * @return Error code
 * @attention This function does not free physical memory
*/
EXTERN STATUS MmUnmapMemoryEx(uintptr_t vAddress, uintptr_t size);

EXPORT
/**
 * @brief Create and prepare page directory for a process
 * 
 * This function copies page directory kernel space mappings.
 * @return Page directory physical address or 0 on failure
*/
EXTERN uintptr_t MmCreateProcessPageDirectory(void);

/**
 * @brief Get current page directory physical address
 * @return Page directory physical address
*/
uintptr_t MmGetPageDirectoryAddress(void);

/**
 * @brief Switch current page directory
 * @param pageDir New page directory physical address
 * @warning Use exclusively when absolutely neccessary
*/
void MmSwitchPageDirectory(uintptr_t pageDir);

/**
 * @}
*/

#endif