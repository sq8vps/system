//This header file is generated automatically
#ifndef EXPORTED___API__MM_VALLOC_H_
#define EXPORTED___API__MM_VALLOC_H_

#ifdef __cplusplus
extern "C" 
{
#endif

#include <stdint.h>
#include "defines.h"
#include "../cdefines.h"
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
 * @brief Get page table entry flags
 * @param vAddress Input virtual address
 * @param *flags Output flags
 * @return Status code
*/
extern STATUS MmGetPageFlags(uintptr_t vAddress, MmPagingFlags_t *flags);

/**
 * @brief Get current process physical address corresponding to the virtual address
 * @param vAddress Input virtual address
 * @param *pAddress Output physical address
 * @return Error code
 */
extern STATUS MmGetPhysicalPageAddress(uintptr_t vAddress, uintptr_t *pAddress);

/**
 * @brief Map current process memory
 * @param vAddress Address to map the memory to
 * @param pAddress Physical memory address
 * @param flags Page flags
 * @return Error code
 * @attention This function does not allocate physical memory.
*/
extern STATUS MmMapMemory(uintptr_t vAddress, uintptr_t pAddress, MmPagingFlags_t flags);

/**
 * @brief Map multiple pages of current process memory
 * @param vAddress Address to map the memory to
 * @param pAddress Physical memory address
 * @param size Memory size in bytes
 * @param flags Page flags
 * @return Error code
 * @attention This function does not allocate physical memory.
*/
extern STATUS MmMapMemoryEx(uintptr_t vAddress, uintptr_t pAddress, uintptr_t size, MmPagingFlags_t flags);

/**
 * @brief Unmap current process memory
 * @param vAddress Address to map the memory to
 * @return Error code
 * @attention This function does not free physical memory.
*/
extern STATUS MmUnmapMemory(uintptr_t vAddress);

/**
 * @brief Unmap multiple pages of current process memory
 * @param vAddress Address to unmap
 * @param size Memory size in bytes
 * @return Error code
 * @attention This function does not free physical memory
*/
extern STATUS MmUnmapMemoryEx(uintptr_t vAddress, uintptr_t size);

/**
 * @brief Create and prepare page directory for a process
 * 
 * This function copies page directory kernel space mappings.
 * @return Page directory physical address or 0 on failure
*/
extern uintptr_t MmCreateProcessPageDirectory(void);


#ifdef __cplusplus
}
#endif

#endif