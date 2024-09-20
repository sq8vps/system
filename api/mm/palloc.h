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
 * @brief Physical memory pool types
 */
enum
{
    MM_PHYSICAL_POOL_STANDARD = 0, /**< Standard pool for general use */
    //additional pools might be defined by the architecture-specific code
};



/**
 * @brief Allocate ONE physical memory block (best fit) from standard pool
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
PSIZE MmAllocatePhysicalMemory(PSIZE n, PADDRESS *address);


/**
 * @brief Allocate ONE physical memory block (best fit) from given pool
 * 
 * This function allocates exactly ONE memory block. The number of requested bytes and actually allocated bytes differs when
 * the biggest available block is too small. In such scenario that smaller block -is- allocated anyway 
 * and the higher-level function should look for next non-contiguous memory blocks to fulfill the request.
 * This also happens when the requested size is bigger than the biggest block size, even if
 * there are more contiguous block (there is just no higher order block).
 * Of course when there is not a single lowest order block available nothing is allocated and 0 is returned.
 * @param n Byte count
 * @param *address Allocated physical memory address
 * @param pool Physical memory pool to allocate from
 * @return Number of bytes actually allocated. 0 when memory is full.
 * @warning This is a very low level utility and should not be used by anything but the memory allocator
*/
PSIZE MmAllocatePhysicalMemoryFromPool(PSIZE n, PADDRESS *address, uint32_t pool);

/**
 * @brief Allocate contiguous physical memory from standard pool
 * @param n Byte Count
 * @param *address Allocated physical memory address
 * @param align Requested alignment in bytes
 * @return Number of bytes actually allocated. 0 when no contiguous block of size \a n was found.
 * @warning This function is slow and should not be used when not needed.
*/
PSIZE MmAllocateContiguousPhysicalMemory(PSIZE n, PADDRESS *address, PSIZE align);

/**
 * @brief Allocate contiguous physical memory from given pool
 * @param n Byte Count
 * @param *address Allocated physical memory address
 * @param align Requested alignment in bytes
 * @param pool Physical memory pool to allocate from
 * @return Number of bytes actually allocated. 0 when no contiguous block of size \a n was found.
 * @warning This function is slow and should not be used when not needed.
*/
PSIZE MmAllocateContiguousPhysicalMemoryFromPool(PSIZE n, PADDRESS *address, PSIZE align, uint32_t pool);

/**
 * @brief Free physical memory
 * @param address Starting address
 * @param n Byte count. Freeing 0 bytes has no effect
 * @warning It is a caller's responsibility to free non-contiguous memory fragment by fragment
*/
void MmFreePhysicalMemory(PADDRESS address, PSIZE n);

/**
 * @brief Check if given physical memory region is usable at all
 * @param address Region start address
 * @param size Region size
 * @return True if usable, false otherwise
*/
bool MmCheckIfPhysicalMemoryUsable(PADDRESS address, PSIZE size);

/**
 * @brief Check if given physical memory region fits in given pool
 * @param address Region start address
 * @param size Region size
 * @param pool Physical memory pool
 * @return True if usable, false otherwise
*/
bool MmCheckIfPhysicalMemoryInPool(PADDRESS address, PSIZE size, uint32_t pool);


#ifdef __cplusplus
}
#endif

#endif