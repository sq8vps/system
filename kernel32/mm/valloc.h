#ifndef KERNEL32_VALLOC_H_
#define KERNEL32_VALLOC_H_

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
#include "../defines.h"
#include "../../cdefines.h"

/**
 * @defgroup vMem Virtual memory module
 * @ingroup mm
 * @{
*/

typedef uint32_t MmPageDirectoryEntry_t;
typedef uint32_t MmPageTableEntry_t;
typedef uint16_t MmPagingFlags_t;

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
STATUS MmInitVirtualAllocator(void);

/**
 * @brief Get current process physical address corresponding to the virtual address
 * @param vAddress Input virtual address
 * @param *pAddress Output physical address
 * @return Error code
 */
EXPORT STATUS MmGetPhysicalPageAddress(uintptr_t vAddress, uintptr_t *pAddress);

/**
 * @brief Map current process memory
 * @param vAddress Address to map the memory to
 * @param pAddress Physical memory address
 * @param flags Page flags
 * @return Error code
 * @attention This function does not allocate physical memory.
*/
EXPORT STATUS MmMapMemory(uintptr_t vAddress, uintptr_t pAddress, MmPagingFlags_t flags);

/**
 * @brief Map multiple pages of current process memory
 * @param vAddress Address to map the memory to
 * @param pAddress Physical memory address
 * @param size Memory size in bytes
 * @param flags Page flags
 * @return Error code
 * @attention This function does not allocate physical memory.
*/
EXPORT STATUS MmMapMemoryEx(uintptr_t vAddress, uintptr_t pAddress, uintptr_t size, MmPagingFlags_t flags);

/**
 * @brief Unmap current process memory
 * @param vAddress Address to map the memory to
 * @return Error code
 * @attention This function does not free physical memory.
*/
EXPORT STATUS MmUnmapMemory(uintptr_t vAddress);

/**
 * @brief Unmap multiple pages of current process memory
 * @param vAddress Address to unmap
 * @param size Memory size in bytes
 * @return Error code
 * @attention This function does not free physical memory
*/
EXPORT STATUS MmUnmapMemoryEx(uintptr_t vAddress, uintptr_t size);

/**
 * @brief Create and prepare page directory for a process
 * @return Page directory physical address or 0 on failure
 * 
 * 
 * This function copies page directory kernel space mappings.
*/
EXPORT uintptr_t MmCreateProcessPageDirectory(void);

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