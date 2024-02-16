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