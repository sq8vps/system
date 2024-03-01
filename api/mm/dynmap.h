//This header file is generated automatically
#ifndef EXPORTED___API__MM_DYNMAP_H_
#define EXPORTED___API__MM_DYNMAP_H_

#ifdef __cplusplus
extern "C" 
{
#endif

#include <stdint.h>
#include "../cdefines.h"
#include "defines.h"
#include "valloc.h"
/**
 * @brief Reserve dynamic memory pool without mapping
 * @param n Byte count
 * @return Pointer to reserved virtual memory
 * @attention This function does not map the memory. 
 * The pointer does not point to any physical memory. 
 * To reserve and map dynamic memory, use \a MmMapDynamicMemory()
*/
extern void *MmReserveDynamicMemory(uintptr_t n);

/**
 * @brief Free reservation of dynamic memory pool
 * @param *ptr Memory pointer from \a MmReserveDynamicMemory()
 * @return Count of bytes previously reserved
 * @attention This function does not unmap the memory.
*/
extern uintptr_t MmFreeDynamicMemoryReservation(void *ptr);

/**
 * @brief Map dynamic kernel memory
 * @param pAddress Physical address
 * @param n Byte count
 * @param flags Flags to apply to mapped memory (present and writable flags are always added)
 * @return Pointer to mapped virtual memory
*/
extern void *MmMapDynamicMemory(uintptr_t pAddress, uintptr_t n, MmPagingFlags_t flags);

/**
 * @brief Unmap dynamic kernel memory
 * @param *ptr Memory pointer (from MmMapDynamicMemory)
*/
extern void MmUnmapDynamicMemory(void *ptr);


#ifdef __cplusplus
}
#endif

#endif