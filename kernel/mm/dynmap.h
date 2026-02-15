#ifndef KERNEL_DYNMAP_H_
#define KERNEL_DYNMAP_H_

/**
 * @file dynmap.h
 * @brief Dynamically mapped kernel memory suppoty
 * 
 * Provides routines for mapping dynamic kernel memory to IO or preallocated regions.
 * Similar to MmAllocateKernelHeap, but does not allocate physical memory.
 * @ingroup mm_dynmap
*/


#include <stdint.h>
#include "defines.h"
#include "mm/mm.h"

/**
 * @addtogroup mm_dynmap Dynamically mapped kernel memory routines
 * @ingroup mm
 * @{
*/

DRIVER_API

/**
 * @brief Reserve dynamic memory pool without mapping
 * @param n Byte count
 * @return Pointer to reserved virtual memory
 * @attention This function does not map the memory. 
 * The pointer does not point to any physical memory. 
 * To reserve and map dynamic memory, use \a MmMapDynamicMemory()
*/
void *MmReserveDynamicMemory(size_t n);


/**
 * @brief Free reservation of dynamic memory pool
 * @param *ptr Memory pointer from \a MmReserveDynamicMemory()
 * @return Count of bytes previously reserved
 * @attention This function does not unmap the memory.
*/
size_t MmFreeDynamicMemoryReservation(const void *ptr);


/**
 * @brief Map dynamic kernel memory
 * @param pAddress Physical address
 * @param n Byte count
 * @param flags Flags to apply to mapped memory (present and writable flags are always added)
 * @return Pointer to mapped virtual memory
*/
void *MmMapDynamicMemory(PADDRESS pAddress, size_t n, MmMemoryFlags flags);


/**
 * @brief Unmap dynamic kernel memory
 * @param *ptr Memory pointer (from MmMapDynamicMemory)
*/
void MmUnmapDynamicMemory(const void *ptr);

END_DRIVER_API

/**
 * @brief Initialize dynamic kernel memory module
 * @kinternal
*/
INTERNAL void MmInitDynamicMemory();

/**
 * @}
*/

#endif