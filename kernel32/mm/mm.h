#ifndef KERNEL32_MM_H_
#define KERNEL32_MM_H_

/**
 * @file mm.h
 * @brief Kernel memory management module
 * 
 * This is the lowest-level memory management utility.
 * Provides physical memory allocation, physical to virtual mapping
 * and kernel heap allocation. Does not provide per-process memory management.
 * 
 * @defgroup mm Memory management
*/


#include <stdint.h>
#include "../defines.h"

/**
 * @defgroup llMm Low-level memory management routines
 * @ingroup mm
 * @{
*/


#define MM_PAGE_SIZE 4096 //memory page size in bytes
#define MM_KERNEL_HEAP_START 0xD8000000 //kernel heap start address
#define MM_KERNEL_HEAP_MAX_SIZE 0x10000000 //kernel heap max size

/**
 * @brief Flags for page directories and page table entries
*/
#define MM_PAGE_FLAG_PRESENT 1
#define MM_PAGE_FLAG_WRITABLE 2
#define MM_PAGE_FLAG_USER 4
#define MM_PAGE_FLAG_PWT 8
#define MM_PAGE_FLAG_PCD 16


/**
 * @brief Initialize memory management module
 * @param usageTable Page usage table address (created by bootloader)
 * @param pageDir Kernel page directory address (created by bootloader)
 * @attention Paging must be initialized, configured and started by the bootloader
 * @return Error code
 */
kError_t Mm_init(uint32_t usageTable, uint32_t pageDir);

/**
 * @brief Create new page directory and map highest page table to page directory itself (self-referencing trick)
 * @param *pageDir Returned page directory physical address
 * @return Error code
 */
kError_t Mm_createPageDir(uint32_t *pageDir);


/**
 * @brief Get physical address corresponding to the virtual address
 * @param vAddress Input virtual address
 * @param *pAddress Output physical address
 * @return Error code
 */
kError_t Mm_getPhysAddr(uint32_t vAddress, uint32_t *pAddress);

/**
 * @brief Map physical address to virtual address
 * @param vAddress Virtual address
 * @param pAddress Physical address
 * @param flags Flags for the mapped page. The "PRESENT" flag is always added.
 * @return Error code
 */
kError_t Mm_mapPage(uint32_t vAddress, uint32_t pAddress, uint8_t flags);

/**
 * @brief Allocate memory page and map it to virtual address
 * @param vAddress Virtual address
 * @param flags Flags for the mapped page. The "PRESENT" flag is always added.
 * @return Error code
 */
kError_t Mm_allocate(uint32_t vAddress, uint8_t flags);

/**
 * @brief Allocate specified number of memory pages and map them to virtual addresses
 * @param vAddress Starting virtual address
 * @param pages Number of pages
 * @param flags Flags for the mapped pages. The "PRESENT" flag is always added.
 * @return Error code
 */
kError_t Mm_allocateEx(uint32_t vAddress, uint32_t pages, uint8_t flags);

/**
 * @brief Allocate and map a block of physically contiguous pages
 * @param pages Number of pages to allocate
 * @param vAddress Starting virtual address
 * @param flags Page flags. The "PRESENT" flag is always added.
 * @param align Alignment value in pages (0 or 1 for 1 page alignment)
 * @return Error code
 */
kError_t Mm_allocateContiguous(uint32_t pages, uint32_t vAddress, uint8_t flags, uint8_t align);

/**
 * @}
*/

#endif
