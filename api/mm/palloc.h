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
 * @brief Allocate ONE physical memory block (best fit)
 * 
 * This function allocates exactly ONE memory block. The number of requested bytes and actually allocated bytes differs when
 * the biggest available block is too small. In such scenario that smaller block -is- allocated anyway 
 * and the higher-level function should look for next non-contiguous memory blocks to fulfill the request.
 * This also happens when the requested size is bigger than the biggest block size, even if
 * there are more contiguous block (there is just no higher order block).
 * Of course when there is not a single lowest order block available nothing is allocated and 0 is returned.
 * @param n Byte count
 * @param *address Allocated physical memory address
 * @return Number of bytes actually allocated. 0 when memory is full.
 * @warning This is a very low level utility and should not be used by anything but the memory allocator
*/
uintptr_t MmAllocatePhysicalMemory(uintptr_t n, uintptr_t *address);


/**
 * @brief Allocate contiguous physical memory
 * @param n Byte Count
 * @param *address Allocated physical memory address
 * @param align Requested alignment in bytes
 * @return Number of bytes actually allocated. 0 when no contiguous block of size n was found.
 * @warning This function is slow and should not be used when not needed.
*/
uintptr_t MmAllocateContiguousPhysicalMemory(uintptr_t n, uintptr_t *address, uintptr_t align);


/**
 * @brief Free physical memory
 * @param address Starting address
 * @param n Byte count. Freeing 0 bytes has no effect
 * @warning It is a caller's responsibility to free non-contiguous memory fragment by fragment
*/
void MmFreePhysicalMemory(uintptr_t address, uintptr_t n);



/**
 * @brief Check if given physical memory region is usable at all
 * @param address Region start address
 * @param size Region size
 * @return 1 if usable, 0 if not
*/
bool MmCheckIfPhysicalMemoryUsable(uintptr_t address, uintptr_t size);


#ifdef __cplusplus
}
#endif

#endif