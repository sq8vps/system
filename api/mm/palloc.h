//This header file is generated automatically
#ifndef EXPORTED___API__MM_PALLOC_H_
#define EXPORTED___API__MM_PALLOC_H_

#ifdef __cplusplus
extern "C" 
{
#endif

#include <stdint.h>
#include <stdbool.h>
#include "../cdefines.h"
#include "defines.h"
/**
 * @brief Allocate contiguous physical memory
 * @param n Byte Count
 * @param *address Allocated physical memory address
 * @param align Requested alignment in bytes
 * @return Number of bytes actually allocated. 0 when no contiguous block of size n was found.
 * @warning This function is slow and should not be used when not needed.
*/
extern uintptr_t MmAllocateContiguousPhysicalMemory(uintptr_t n, uintptr_t *address, uintptr_t align);

/**
 * @brief Free physical memory
 * @param address Starting address
 * @param n Byte count. Freeing 0 bytes has no effect
 * @warning It is a caller's responsibility to free non-contiguous memory fragment by fragment
*/
extern void MmFreePhysicalMemory(uintptr_t address, uintptr_t n);


#ifdef __cplusplus
}
#endif

#endif