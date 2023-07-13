#ifndef LOADER_MM_H
#define LOADER_MM_H

#include <stdint.h>
#include "defines.h"

#define MM_PAGE_SIZE 4096

#define MM_BIOS_MEMORY_MAP (LOADER_DATA + 64) //memory address where memory map is stored
#define MM_BIOS_MEMORY_MAP_ENTRY_SIZE 24 //size of one memory map entry (depends on the 2nd stage bootloader)
#define MM_BIOS_MEMORY_MAP_MAX_ENTRY_COUNT 256 //max memory map entries count

#define MM_PAGE_FLAG_PRESENT 1
#define MM_PAGE_FLAG_WRITABLE 2
#define MM_PAGE_FLAG_USER 4
#define MM_PAGE_FLAG_PWT 8
#define MM_PAGE_FLAG_PCD 16


/**
 * @brief Initialize memory management module
 * @param *pageDir Returned page directory physical address
 *
 * Scan for free memory regions, build and initialize page frame usage bitmap, prepare initial page directory and set identity paging for the lowest 1 MiB.
 * After initialization, paging may be safely enabled.
 */
error_t Mm_init(uint32_t *pageDir);

/**
 * @brief Create new page directory and map highest page table to page directory itself (self-referencing trick)
 * @param *pageDir Returned page directory physical address
 */
error_t Mm_newPageDir(uint32_t *pageDir);

/**
 * @brief Enable paging
 * @param pageDir Page directory address
 */
void Mm_enablePaging(uint32_t pageDir);

/**
 * @brief Get physical address corresponding to the virtual address
 * @param vAddress Input virtual address
 * @param *pAddress Output physical address
 */
error_t Mm_getPhysAddr(uint32_t vAddress, uint32_t *pAddress);

/**
 * @brief Map physical address to virtual address
 * @param vAddress Virtual address
 * @param pAddress Physical address
 * @param flags Flags for the mapped page
 */
error_t Mm_mapPage(uint32_t vAddress, uint32_t pAddress, uint8_t flags);

/**
 * @brief Allocate memory page and map it to virtual address
 * @param vAddress Virtual address
 * @param flags Flags for the mapped page
 */
error_t Mm_allocate(uint32_t vAddress, uint8_t flags);

/**
 * @brief Allocate specified number of memory pages and map them to virtual addresses
 * @param vAddress Starting virtual address
 * @param pages Number of pages
 * @param flags Flags for the mapped pages
 */
error_t Mm_allocateEx(uint32_t vAddress, uint32_t pages, uint8_t flags);

/**
 * @brief Allocate and map a block of physically contiguous pages
 * @param pages Number of pages to allocate
 * @param vAddress Starting virtual address
 * @param flags Page flags
 * @param align Alignment value in pages (0 or 1 for 1 page alignment)
 */
error_t Mm_allocateContiguous(uint32_t pages, uint32_t vAddress, uint8_t flags, uint8_t align);

#endif
