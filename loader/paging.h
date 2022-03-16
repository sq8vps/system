#ifndef LOADER_PAGING_H
#define LOADER_PAGING_H

#include <stdint.h>

/**
 * @brief Construct temporary page tables and page directory
 * @param kernelPhysicalAddress Physical address of kernel - 4 KiB aligned
 * @param kernelVirtualAddress Virtual address of kernel (where should kernel be mapped) - 4 MiB aligned
 * @attention Address alignment is not checked. Improper alignment results in undefined behavior.
 *
 * This function constructs temporary page tables for kernel, identity pages the first 1 MiB and sets up page directory.
 * That makes it possible for the kernel to run, i.e. in the higher half, while keeps the bootloader running properly.
 * The kernel should provide it's own page directory and page tables.
 *
 */
void Paging_construct(uint32_t kernelPhysicalAddress, uint32_t kernelVirtualAddress);

/**
 * @brief Enable paging
 * @attention Page tables and page directory must be constructed first using Paging_construct()
 */
void Paging_enable(void);



#endif
