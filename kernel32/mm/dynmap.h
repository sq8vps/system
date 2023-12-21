#ifndef KERNEL_DYNMAP_H_
#define KERNEL_DYNMAP_H_

/**
 * @file dynmap.h
 * @brief Dynamically mapped kernel memory module
 * 
 * Provides routines for mapping dynamic kernel memory to IO or preallocated regions.
 * Similar to MmAllocateKernelHeap, but does not allocate physical memory.
 * @ingroup mm
*/


#include <stdint.h>
#include "../cdefines.h"
#include "defines.h"
#include "valloc.h"

/**
 * @defgroup dynmap Dynamically mapped kernel memory routines
 * @ingroup mm
 * @{
*/

EXPORT
/**
 * @brief Map dynamic kernel memory
 * @param pAddress Physical address
 * @param n Byte count
 * @param flags Flags to apply to mapped memory (present and writable flags are always added)
 * @return Pointer to mapped virtual memory
*/
EXTERN void *MmMapDynamicMemory(uintptr_t pAddress, uintptr_t n, MmPagingFlags_t flags);

EXPORT
/**
 * @brief Unmap dynamic kernel memory
 * @param *ptr Memory pointer (from MmMapDynamicMemory)
 * @param n Byte count
*/
EXTERN void MmUnmapDynamicMemory(void *ptr, uintptr_t n);

/**
 * @brief Initialize dynamic kernel memory module
 * @param *kernelArgs Kernel entry arguments
 * @warning Prepares trees taking into account space reserved for initial ramdisk.
*/
void MmInitDynamicMemory(struct KernelEntryArgs *kernelArgs);

/**
 * @}
*/

#endif